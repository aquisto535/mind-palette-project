#pragma once
#include "crow.h"
#include "image_processor.h"
#include "utils/Logger.h"
#include <filesystem>
#include <string>
#include <chrono>

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
        
        auto body = crow::json::load(req.body);
        if (!body) {
            LOG_ERROR("Invalid JSON body received");
            return crow::response(400, "Invalid JSON");
        }

        // Check if imagePath exists in JSON
        if (!body.has("imagePath")) {
            LOG_ERROR("Missing imagePath in request body");
            return crow::response(400, "Missing imagePath");
        }
        
        // Check if imagePath is empty
        std::string imagePath = body["imagePath"].s();
        if (imagePath.empty()) {
            LOG_ERROR("imagePath is empty");
            return crow::response(400, "imagePath is empty");
        }
        
        // Check if file exists
        if (!fs::exists(imagePath)) {
            LOG_ERROR("File not found: {}", imagePath);
            return crow::response(404, "File not found");
        }
        
        LOG_INFO("Processing file: {}", imagePath);
        
        // Process the image
        ImageProcessor processor;
        cv::Mat img = processor.Load(imagePath);
        if (img.empty()) {
            LOG_ERROR("Failed to load image: {}", imagePath);
            return crow::response(400, "Failed to load image");
        }
        
        cv::Mat processed = processor.Preprocess(img);
        if (processed.empty()) {
            LOG_ERROR("Preprocessing failed for: {}", imagePath);
            return crow::response(500, "Preprocessing failed");
        }
        
        // Generate output path and save
        std::string outputPath = GenerateOutputPath(imagePath);
        
        // Create output directory if not exists
        fs::path outputDir = fs::path(outputPath).parent_path();
        if (!fs::exists(outputDir)) {
            fs::create_directories(outputDir);
        }
        
        if (!processor.Save(processed, outputPath)) {
            LOG_ERROR("Failed to save processed image to: {}", outputPath);
            return crow::response(500, "Failed to save processed image");
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        LOG_INFO("Successfully processed image in {}ms. Saved to: {}", duration, outputPath);
        
        crow::json::wvalue res;
        res["processedPath"] = outputPath;
        
        return crow::response(200, res);
    });
}

