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
    crow::request req;
    req.body = R"({"imagePath": "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\uploads\\wrtFileImageView.jpg"})";
    
    auto result = ValidatePreprocessRequest(req);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.imagePath, "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\uploads\\wrtFileImageView.jpg");
    EXPECT_EQ(result.errorCode, 200);
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

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

