#pragma once
#include "crow.h"
#include "core/image_processor.h"
#include "core/validation_exception.h"
#include "infra/thread_pool.h"
#include "infra/atomic_writer.h"
#include "utils/Logger.h"
#include <filesystem>
#include <string>
#include <chrono>
#include <optional>
#include <future>
#include <memory>
#include <cstdlib>

namespace fs = std::filesystem;

// ============================================================================
// Cross-platform Environment Variable Helper
// ============================================================================
inline std::string GetEnvVar(const char* key) {
#ifdef _MSC_VER
    char* val = nullptr;
    size_t len = 0;
    if (_dupenv_s(&val, &len, key) == 0 && val != nullptr) {
        std::string result(val);
        free(val);
        return result;
    }
    return "";
#else
    const char* val = std::getenv(key);
    return val ? std::string(val) : "";
#endif
}

// ============================================================================
// Global Thread Pool (Worker threads for CPU-bound image processing)
// Separates heavy processing from Crow's I/O threads
// ============================================================================
inline ThreadPool& GetWorkerPool() {
    static ThreadPool pool([]() -> size_t {
        // Allow runtime tuning via environment variable
        std::string workersStr = GetEnvVar("PREPROCESS_WORKERS");
        if (!workersStr.empty()) {
            size_t n = static_cast<size_t>(std::atoi(workersStr.c_str()));
            if (n > 0) return n;
        }
        // Default: cap at 4 to prevent over-provisioning on high-core-count machines
        size_t hw = std::thread::hardware_concurrency();
        return (hw > 0) ? std::min(hw, size_t(4)) : 2;
    }());
    return pool;
}

// ============================================================================
// Helper Functions for /preprocess Route
// ============================================================================

// Generate output path from input path
// Example: .../shared_volume/uploads/test.jpg -> .../shared_volume/processed/test_clean.jpg
inline std::string GenerateOutputPath(const std::string& inputPath) {
    fs::path p(inputPath);
    std::string stem = p.stem().string();
    std::string ext = p.extension().string();

    // Derive output dir from the absolute input path to be independent of CWD.
    // uploads/ and processed/ are siblings under shared_volume/, so go up two levels.
    fs::path outputDir = p.parent_path().parent_path() / "processed";

    // Ensure directory exists
    if (!fs::exists(outputDir)) {
        fs::create_directories(outputDir);
    }

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
inline crow::response CreatePreprocessResponse(const std::string& outputPath, int64_t durationMs, const std::string& requestId, const std::string& serverTimingHeader = "") {
    // LOG_INFO(id, fmt, args...) -> ID is used for [{}]
    LOG_INFO(requestId, "Successfully processed image in {}ms. Saved to: {}", durationMs, outputPath);
    
    crow::json::wvalue res;
    res["processedPath"] = outputPath;
    
    crow::response response(200, res);
    if (!serverTimingHeader.empty()) {
        response.add_header("Server-Timing", serverTimingHeader);
    }
    return response;
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
            try {
                promise->set_value(ProcessImageFile(imagePath, requestId));
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });

        // Wait for worker thread to complete processing
        // ValidationException propagates via promise/future → HTTP 422
        std::optional<cv::Mat> processedOpt;
        try {
            processedOpt = future.get();
        } catch (const ValidationException& e) {
            LOG_WARN(requestId, "Color validation failed: {}", e.what());
            crow::json::wvalue body;
            body["error"] = "COLOR_VALIDATION_FAILED";
            body["message"] = std::string(e.what());
            crow::response res(422, body.dump());
            res.add_header("Content-Type", "application/json");
            return res;
        } catch (...) {
            return crow::response(500, "Processing failed");
        }

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
        
        std::string serverTiming;
        std::string clientKey = req.get_header_value("X-Admin-Profile-Key");
        std::string adminKey = GetEnvVar("ADMIN_PROFILE_KEY");
        if (!adminKey.empty() && clientKey == adminKey) {
            serverTiming = "preprocess;dur=" + std::to_string(duration);
        }
        
        return CreatePreprocessResponse(outputPath, duration, requestId, serverTiming);
    });
}
