#include <gtest/gtest.h>
#include "crow.h"
#include "server.h"
#include "image_processor.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// ============================================================================
// Server Route Tests
// ============================================================================

TEST(ServerTest, RootRoute) {
    crow::SimpleApp app;
    setup_routes(app);
    app.validate();

    crow::request req;
    req.url = "/";
    
    crow::response res;
    app.handle_full(req, res);
    
    EXPECT_EQ(res.code, 200);
    EXPECT_EQ(res.body, "Preprocess Server is running!");
}

TEST(ServerTest, HealthCheck) {
    crow::SimpleApp app;
    setup_routes(app);
    app.validate();

    crow::request req;
    req.url = "/health";
    
    crow::response res;
    app.handle_full(req, res);
    
    EXPECT_EQ(res.code, 200);
    EXPECT_EQ(res.body, "OK");
}

TEST(ServerTest, Preprocess_MissingImagePath_Returns400) {
    crow::SimpleApp app;
    setup_routes(app);
    app.validate();

    crow::request req;
    req.url = "/preprocess";
    req.method = crow::HTTPMethod::POST;
    req.body = R"({})";
    
    crow::response res;
    app.handle_full(req, res);
    
    EXPECT_EQ(res.code, 400);
}

TEST(ServerTest, Preprocess_EmptyImagePath_Returns400) {
    crow::SimpleApp app;
    setup_routes(app);
    app.validate();

    crow::request req;
    req.url = "/preprocess";
    req.method = crow::HTTPMethod::POST;
    req.body = R"({"imagePath": ""})";
    
    crow::response res;
    app.handle_full(req, res);
    
    EXPECT_EQ(res.code, 400);
}

TEST(ServerTest, Preprocess_NonExistentFile_Returns404) {
    crow::SimpleApp app;
    setup_routes(app);
    app.validate();

    crow::request req;
    req.url = "/preprocess";
    req.method = crow::HTTPMethod::POST;
    req.body = R"({"imagePath": "/nonexistent/path/image.jpg"})";
    
    crow::response res;
    app.handle_full(req, res);
    
    EXPECT_EQ(res.code, 404);
}

// ============================================================================
// Helper Function Unit Tests
// ============================================================================

TEST(ValidatePreprocessRequestTest, ValidRequest_ReturnsSuccess) {
    // Create a dummy file for testing
    std::string tempFileName = "test_image.jpg";
    std::ofstream outfile(tempFileName);
    outfile << "dummy content";
    outfile.close();

    // Get absolute path
    auto absPath = std::filesystem::absolute(tempFileName).string();

    // Fix path separators for JSON (escape backslashes on Windows)
    std::string jsonPath = absPath;
    size_t pos = 0;
    while ((pos = jsonPath.find('\\', pos)) != std::string::npos) {
        jsonPath.replace(pos, 1, "\\\\");
        pos += 2;
    }

    crow::request req;
    req.body = "{\"imagePath\": \"" + jsonPath + "\"}";
    
    auto result = ValidatePreprocessRequest(req);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.imagePath, absPath);
    EXPECT_EQ(result.errorCode, 200);

    // Cleanup
    std::filesystem::remove(tempFileName);
}

TEST(ValidatePreprocessRequestTest, InvalidJSON_Returns400) {
    crow::request req;
    req.body = "invalid json";
    
    auto result = ValidatePreprocessRequest(req);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorCode, 400);
    EXPECT_EQ(result.errorMessage, "Invalid JSON");
}

TEST(ValidatePreprocessRequestTest, MissingImagePath_Returns400) {
    crow::request req;
    req.body = R"({"otherField": "value"})";
    
    auto result = ValidatePreprocessRequest(req);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorCode, 400);
    EXPECT_EQ(result.errorMessage, "Missing imagePath");
}

TEST(ValidatePreprocessRequestTest, EmptyImagePath_Returns400) {
    crow::request req;
    req.body = R"({"imagePath": ""})";
    
    auto result = ValidatePreprocessRequest(req);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorCode, 400);
    EXPECT_EQ(result.errorMessage, "imagePath is empty");
}

TEST(ValidatePreprocessRequestTest, NonExistentFile_Returns404) {
    crow::request req;
    req.body = R"({"imagePath": "/nonexistent/file.jpg"})";
    
    auto result = ValidatePreprocessRequest(req);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorCode, 404);
    EXPECT_EQ(result.errorMessage, "File not found");
}

TEST(CreatePreprocessResponseTest, CreatesValidJsonResponse) {
    auto response = CreatePreprocessResponse("/output/test.jpg", 42);
    
    EXPECT_EQ(response.code, 200);
    
    // Parse response body as JSON
    auto body = crow::json::load(response.body);
    EXPECT_TRUE(body);
    EXPECT_EQ(body["processedPath"].s(), "/output/test.jpg");
}

// ============================================================================
// ImageProcessor Unit Tests
// ============================================================================

class ImageProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test image (100x100 BGR)
        testImage = cv::Mat(100, 100, CV_8UC3, cv::Scalar(255, 128, 64));
    }
    
    cv::Mat testImage;
    ImageProcessor processor;
};

TEST_F(ImageProcessorTest, Preprocess_ResizesTo512x512) {
    cv::Mat result = processor.Preprocess(testImage);
    
    EXPECT_EQ(result.rows, 512);
    EXPECT_EQ(result.cols, 512);
}

TEST_F(ImageProcessorTest, Preprocess_OutputIsGrayscale) {
    cv::Mat result = processor.Preprocess(testImage);
    
    // Grayscale image has 1 channel
    EXPECT_EQ(result.channels(), 1);
}

TEST_F(ImageProcessorTest, Preprocess_EmptyInputReturnsEmpty) {
    cv::Mat empty;
    cv::Mat result = processor.Preprocess(empty);
    
    EXPECT_TRUE(result.empty());
}

TEST_F(ImageProcessorTest, Load_NonExistentFileReturnsEmpty) {
    cv::Mat result = processor.Load("/nonexistent/image.jpg");
    
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// GenerateOutputPath Unit Tests
// ============================================================================

TEST(GenerateOutputPathTest, GeneratesCorrectPath) {
    std::string input = "/shared/uploads/test.jpg";
    std::string expected = "/shared/processed/test_clean.jpg";
    
    std::string result = GenerateOutputPath(input);
    
    // Normalize path separators for cross-platform
    std::replace(result.begin(), result.end(), '\\', '/');
    EXPECT_EQ(result, expected);
}

TEST(GenerateOutputPathTest, PreservesExtension) {
    std::string input = "/shared/uploads/photo.png";
    std::string result = GenerateOutputPath(input);
    
    EXPECT_TRUE(result.find(".png") != std::string::npos);
}

// ============================================================================
// Week 3: Advanced Image Processing Tests (TDD Red Phase)
// ============================================================================

class AdvancedImageProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test image with distinct foreground (center) and background (edges)
        // Simulates a child's drawing on white paper
        testImage = cv::Mat(512, 512, CV_8UC3, cv::Scalar(255, 255, 255)); // White background
        
        // Draw a colored rectangle in the center (simulating foreground)
        cv::rectangle(testImage, cv::Rect(100, 100, 312, 312), cv::Scalar(0, 0, 255), -1);
        
        // Create grayscale version for edge detection tests
        cv::cvtColor(testImage, grayscaleImage, cv::COLOR_BGR2GRAY);
    }
    
    cv::Mat testImage;
    cv::Mat grayscaleImage;
    ImageProcessor processor;
};

// --- RemoveBackground Tests ---

#if 0
// [DEAD CODE] Disabled tests for RemoveBackground
TEST_F(AdvancedImageProcessorTest, RemoveBackground_ReturnsNonEmptyMask) {
    // GrabCut should return a valid foreground mask
    cv::Mat mask = processor.RemoveBackground(testImage);
    
    EXPECT_FALSE(mask.empty());
}

TEST_F(AdvancedImageProcessorTest, RemoveBackground_MaskIsSingleChannel) {
    cv::Mat mask = processor.RemoveBackground(testImage);
    
    EXPECT_EQ(mask.channels(), 1);
}

TEST_F(AdvancedImageProcessorTest, RemoveBackground_MaskHasForegroundPixels) {
    cv::Mat mask = processor.RemoveBackground(testImage);
    
    // Count foreground pixels (GC_FGD=1 or GC_PR_FGD=3)
    int fgCount = cv::countNonZero(mask);
    EXPECT_GT(fgCount, 0);
}

TEST_F(AdvancedImageProcessorTest, RemoveBackground_EmptyInputReturnsEmpty) {
    cv::Mat empty;
    cv::Mat mask = processor.RemoveBackground(empty);
    
    EXPECT_TRUE(mask.empty());
}
#endif

// --- DetectEdges Tests ---

TEST_F(AdvancedImageProcessorTest, DetectEdges_ReturnsNonEmptyOutput) {
    cv::Mat edges = processor.DetectEdges(grayscaleImage);
    
    EXPECT_FALSE(edges.empty());
}

TEST_F(AdvancedImageProcessorTest, DetectEdges_OutputIsSingleChannel) {
    cv::Mat edges = processor.DetectEdges(grayscaleImage);
    
    EXPECT_EQ(edges.channels(), 1);
}

TEST_F(AdvancedImageProcessorTest, DetectEdges_DetectsRectangleEdges) {
    cv::Mat edges = processor.DetectEdges(grayscaleImage);
    
    // Rectangle edges should be detected
    int edgePixels = cv::countNonZero(edges);
    EXPECT_GT(edgePixels, 100); // At least some edges detected
}

TEST_F(AdvancedImageProcessorTest, DetectEdges_HigherThresholdFewerEdges) {
    cv::Mat edgesLow = processor.DetectEdges(grayscaleImage, 30, 90);
    cv::Mat edgesHigh = processor.DetectEdges(grayscaleImage, 100, 200);
    
    int countLow = cv::countNonZero(edgesLow);
    int countHigh = cv::countNonZero(edgesHigh);
    
    // Higher threshold should detect fewer edges
    EXPECT_GE(countLow, countHigh);
}

// --- EnhanceContours Tests ---

TEST_F(AdvancedImageProcessorTest, EnhanceContours_ReturnsNonEmptyOutput) {
    // Create simple binary image for morphology test
    cv::Mat binary = cv::Mat::zeros(512, 512, CV_8UC1);
    cv::rectangle(binary, cv::Rect(100, 100, 312, 312), cv::Scalar(255), 1);
    
    cv::Mat enhanced = processor.EnhanceContours(binary);
    
    EXPECT_FALSE(enhanced.empty());
}

TEST_F(AdvancedImageProcessorTest, EnhanceContours_ClosesSmallGaps) {
    // Create binary image with a small gap
    cv::Mat binary = cv::Mat::zeros(512, 512, CV_8UC1);
    cv::line(binary, cv::Point(100, 256), cv::Point(250, 256), cv::Scalar(255), 2);
    cv::line(binary, cv::Point(254, 256), cv::Point(400, 256), cv::Scalar(255), 2); // 4px gap
    
    int beforeCount = cv::countNonZero(binary);
    cv::Mat enhanced = processor.EnhanceContours(binary, 5);
    int afterCount = cv::countNonZero(enhanced);
    
    // MORPH_CLOSE should fill the gap, resulting in more or equal white pixels
    EXPECT_GE(afterCount, beforeCount);
}

// --- Binarize Tests ---

TEST_F(AdvancedImageProcessorTest, Binarize_ReturnsNonEmptyOutput) {
    cv::Mat binary = processor.Binarize(grayscaleImage);
    
    EXPECT_FALSE(binary.empty());
}

TEST_F(AdvancedImageProcessorTest, Binarize_OutputIsSingleChannel) {
    cv::Mat binary = processor.Binarize(grayscaleImage);
    
    EXPECT_EQ(binary.channels(), 1);
}

TEST_F(AdvancedImageProcessorTest, Binarize_OnlyContainsBinaryValues) {
    cv::Mat binary = processor.Binarize(grayscaleImage);
    
    // All pixel values should be either 0 or 255
    double minVal, maxVal;
    cv::minMaxLoc(binary, &minVal, &maxVal);
    
    EXPECT_GE(minVal, 0);
    EXPECT_LE(maxVal, 255);
    
    // Check that non-zero values are exactly 255
    cv::Mat nonZero;
    cv::threshold(binary, nonZero, 1, 255, cv::THRESH_BINARY);
    cv::Mat diff;
    cv::absdiff(binary, nonZero, diff);
    EXPECT_EQ(cv::countNonZero(diff), 0);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

