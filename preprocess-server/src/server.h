#pragma once
#include "crow.h"
#include "image_processor.h"
#include "utils/Logger.h"
#include <filesystem>
#include <string>
#include <chrono>
#include <optional>

namespace fs = std::filesystem;

// Generate output path from input path
// /shared/uploads/test.jpg -> /shared/processed/test_clean.jpg
inline std::string GenerateOutputPath(const std::string& inputPath) {
    fs::path p(inputPath);
    std::string stem = p.stem().string();
    std::string ext = p.extension().string();
    fs::path outputDir = "/shared/processed";
    return (outputDir / (stem + "_clean" + ext)).string();
}

// ============================================================================
// Helper Functions for /preprocess Route
// ============================================================================

// Validation result with error details
struct ValidationResult {
    bool success;
    std::string imagePath;
    int errorCode;         // HTTP status code (400, 404, etc.)
    std::string errorMessage;
};

// Returns validation result with appropriate error codes
inline ValidationResult ValidatePreprocessRequest(const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body) {
        LOG_ERROR("Invalid JSON body received");
        return {false, "", 400, "Invalid JSON"};
    }
    
    if (!body.has("imagePath")) {
        LOG_ERROR("Missing imagePath in request body");
        return {false, "", 400, "Missing imagePath"};
    }
    
    std::string imagePath = body["imagePath"].s();
    if (imagePath.empty()) {
        LOG_ERROR("imagePath is empty");
        return {false, "", 400, "imagePath is empty"};
    }
    
    if (!fs::exists(imagePath)) {
        LOG_ERROR("File not found: {}", imagePath);
        return {false, "", 404, "File not found"};
    }
    
    return {true, imagePath, 200, ""};
}

// Returns processed image or nullopt on failure
// Pipeline: Preprocess → Canny → Morphology → Binarize (GrabCut excluded for performance)
inline std::optional<cv::Mat> ProcessImageFile(const std::string& imagePath) {
    LOG_INFO("Processing file: {}", imagePath);
    
    ImageProcessor processor;
    cv::Mat img = processor.Load(imagePath);
    if (img.empty()) {
        LOG_ERROR("Failed to load image: {}", imagePath);
        return std::nullopt;
    }
    
    // Step 1: Preprocess (Letterbox resize + Denoise + Grayscale)
    cv::Mat preprocessed = processor.Preprocess(img);
    if (preprocessed.empty()) {
        LOG_ERROR("Preprocessing failed for: {}", imagePath);
        return std::nullopt;
    }
    
    // Step 2: Canny Edge Detection (threshold 50/150)
    cv::Mat edges = processor.DetectEdges(preprocessed, 50, 150);
    if (edges.empty()) {
        LOG_ERROR("Edge detection failed for: {}", imagePath);
        return std::nullopt;
    }
    
    // Step 3: Morphology Enhancement (MORPH_CLOSE, kernelSize=3)
    cv::Mat enhanced = processor.EnhanceContours(edges, 3);
    if (enhanced.empty()) {
        LOG_ERROR("Morphology enhancement failed for: {}", imagePath);
        return std::nullopt;
    }
    
    // Step 4: Adaptive Binarization
    cv::Mat binarized = processor.Binarize(preprocessed);
    if (binarized.empty()) {
        LOG_ERROR("Binarization failed for: {}", imagePath);
        return std::nullopt;
    }
    
    // Step 5: Convert to RGB 3-channel (required for EfficientNet-B2)
    // Binarized image is single-channel, convert to 3-channel for AI model compatibility
    cv::Mat result;
    cv::cvtColor(binarized, result, cv::COLOR_GRAY2BGR);
    
    LOG_INFO("Pipeline complete. Output: {}x{} (3-channel RGB)", result.cols, result.rows);
    return result;
}

// Saves processed image with directory creation, returns success status
inline bool SaveProcessedImage(const cv::Mat& img, const std::string& outputPath) {
    // Create output directory if not exists
    fs::path outputDir = fs::path(outputPath).parent_path();
    if (!fs::exists(outputDir)) {
        fs::create_directories(outputDir);
    }
    
    ImageProcessor processor;
    if (!processor.Save(img, outputPath)) {
        LOG_ERROR("Failed to save processed image to: {}", outputPath);
        return false;
    }
    
    return true;
}

// Creates success response with performance metrics
inline crow::response CreatePreprocessResponse(const std::string& outputPath, int64_t durationMs) {
    LOG_INFO("Successfully processed image in {}ms. Saved to: {}", durationMs, outputPath);
    
    crow::json::wvalue res;
    res["processedPath"] = outputPath;
    
    return crow::response(200, res);
}


inline void setup_routes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/")([](){
        return "Preprocess Server is running!";
    });

    CROW_ROUTE(app, "/health")([](){
        return crow::response(200, "OK");
    });

    CROW_ROUTE(app, "/preprocess").methods(crow::HTTPMethod::POST)([](const crow::request& req){
        LOG_INFO("Received preprocess request");
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Validate request
        auto validation = ValidatePreprocessRequest(req);
        if (!validation.success) {
            return crow::response(validation.errorCode, validation.errorMessage);
        }
        std::string imagePath = validation.imagePath;
        
        // Process image
        auto processedOpt = ProcessImageFile(imagePath);
        if (!processedOpt) {
            return crow::response(500, "Processing failed");
        }
        
        // Save result
        std::string outputPath = GenerateOutputPath(imagePath);
        if (!SaveProcessedImage(*processedOpt, outputPath)) {
            return crow::response(500, "Failed to save processed image");
        }
        
        // Build response
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        return CreatePreprocessResponse(outputPath, duration);
    });
}

