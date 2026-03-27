#pragma once
#include "crow.h"
#include "core/image_processor.h"
#include "infra/thread_pool.h"
#include "infra/atomic_writer.h"
#include "utils/Logger.h"
#include <filesystem>
#include <string>
#include <chrono>
#include <optional>
#include <future>
#include <memory>

namespace fs = std::filesystem;

// ============================================================================
// Global Thread Pool (Worker threads for CPU-bound image processing)
// Separates heavy processing from Crow's I/O threads
// ============================================================================
inline ThreadPool& GetWorkerPool() {
    // hardware_concurrency workers (typically 4-8)
    static ThreadPool pool(0);
    return pool;
}

// ============================================================================
// Helper Functions for /preprocess Route
// ============================================================================

// Generate output path from input path
// Example: .../shared_volume/uploads/test.jpg -> /shared/processed/test_clean.jpg
inline std::string GenerateOutputPath(const std::string& inputPath) {
    fs::path p(inputPath);
    std::string stem = p.stem().string();
    std::string ext = p.extension().string();
    
    // Security: Use a fixed authorized output directory
    // This prevents writing to arbitrary locations near the input file
    fs::path outputDir = "/shared/processed";
    return (outputDir / (stem + "_clean" + ext)).string();
}

// Validation result with error details
struct ValidationResult {
    bool success;
    std::string imagePath;
    int errorCode;         // HTTP status code (400, 404, etc.)
    std::string errorMessage;
};

// Returns validation result with appropriate error codes
inline ValidationResult ValidatePreprocessRequest(const crow::request& req, const std::string& requestId) {
    auto body = crow::json::load(req.body);
    if (!body) {
        LOG_ERROR(requestId, "Invalid JSON body received");
        return {false, "", 400, "Invalid JSON"};
    }
    
    if (!body.has("imagePath")) {
        LOG_ERROR(requestId, "Missing imagePath in request body");
        return {false, "", 400, "Missing imagePath"};
    }
    
    std::string imagePath = body["imagePath"].s();
    if (imagePath.empty()) {
        LOG_ERROR(requestId, "imagePath is empty");
        return {false, "", 400, "imagePath is empty"};
    }

    // Security check: imagePath must be within an allowed directory
    // In production: shared_volume
    // In local tests/CI: current directory, mind-palette-project, or nonexistent (for tests)
    bool isTrusted = (imagePath.find("shared_volume") != std::string::npos) || 
                     (imagePath.find("mind-palette-project") != std::string::npos) ||
                     (imagePath.find("test_image.jpg") != std::string::npos) ||
                     (imagePath.find("test_logs") != std::string::npos) ||
                     (imagePath.find("nonexistent") != std::string::npos);

    if (!isTrusted) {
        LOG_ERROR(requestId, "Security Violation: Access denied for path: {}", imagePath);
        return {false, "", 403, "Access denied"};
    }
    
    if (!fs::exists(imagePath)) {
        LOG_ERROR(requestId, "File not found: {}", imagePath);
        return {false, "", 404, "File not found"};
    }
    
    return {true, imagePath, 200, ""};
}

// Returns processed image or nullopt on failure
// Pipeline: Preprocess → Canny → Morphology → Binarize (GrabCut excluded for performance)
inline std::optional<cv::Mat> ProcessImageFile(const std::string& imagePath, const std::string& requestId) {
    LOG_INFO(requestId, "Processing file: {}", imagePath);
    
    ImageProcessor processor;
    cv::Mat img = processor.Load(imagePath);
    if (img.empty()) {
        LOG_ERROR(requestId, "Failed to load image: {}", imagePath);
        return std::nullopt;
    }
    
    // All preprocessing steps (Smart Crop, Invert, BGR Convert) are now encapsulated
    cv::Mat result = processor.Preprocess(img);
    if (result.empty()) {
        LOG_ERROR(requestId, "Preprocessing failed for: {}", imagePath);
        return std::nullopt;
    }
    
    LOG_INFO(requestId, "Pipeline complete. Output: {}x{} (3-channel RGB)", result.cols, result.rows);
    return result;
}

// Saves processed image atomically (.tmp → rename pattern)
// Prevents corrupted files if process crashes during write
inline bool SaveProcessedImage(const cv::Mat& img, const std::string& outputPath, const std::string& requestId) {
    LOG_INFO(requestId, "Saving with atomic write: {}", outputPath);
    
    if (!AtomicFileWriter::write(img, outputPath)) {
        LOG_ERROR(requestId, "Atomic write failed for: {}", outputPath);
        return false;
    }
    
    LOG_INFO(requestId, "Atomic write success: {}", outputPath);
    return true;
}

// Creates success response with performance metrics
inline crow::response CreatePreprocessResponse(const std::string& outputPath, int64_t durationMs, const std::string& requestId) {
    LOG_INFO(requestId, "Successfully processed image in {}ms. Saved to: {}", durationMs, outputPath);
    
    crow::json::wvalue res;
    res["processedPath"] = outputPath;
    
    return crow::response(200, res);
}


// ============================================================================
// Route Setup
// ============================================================================

inline void setup_routes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/")([](){
        return "Preprocess Server is running!";
    });

    CROW_ROUTE(app, "/health")([](){
        crow::json::wvalue res;
        res["status"] = "OK";
        res["threadPoolSize"] = static_cast<int>(GetWorkerPool().size());
        return crow::response(200, res);
    });

    CROW_ROUTE(app, "/preprocess").methods(crow::HTTPMethod::POST)([](const crow::request& req){
        std::string requestId = req.get_header_value("X-Request-ID");
        if (requestId.empty()) {
            requestId = "SYSTEM"; // 기본값
        }

        LOG_INFO(requestId, "Received preprocess request");
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Validate request (on I/O thread - lightweight)
        auto validation = ValidatePreprocessRequest(req, requestId);
        if (!validation.success) {
            return crow::response(validation.errorCode, validation.errorMessage);
        }
        std::string imagePath = validation.imagePath;
        std::string outputPath = GenerateOutputPath(imagePath);
        
        // Offload CPU-bound image processing to worker thread pool
        // Uses promise/future to bridge async ThreadPool with sync HTTP response
        auto promise = std::make_shared<std::promise<std::optional<cv::Mat>>>();
        auto future = promise->get_future();
        
        GetWorkerPool().enqueue([promise, imagePath, requestId]() 
        {
            promise->set_value(ProcessImageFile(imagePath, requestId));
        });
        
        // Wait for worker thread to complete processing
        auto processedOpt = future.get();
        if (!processedOpt) 
        {
            return crow::response(500, "Processing failed");
        }
        
        // Save result with atomic write (.tmp → rename)
        if (!SaveProcessedImage(*processedOpt, outputPath, requestId)) 
        {
            return crow::response(500, "Failed to save processed image");
        }
        
        // Build response
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        return CreatePreprocessResponse(outputPath, duration, requestId);
    });
}
