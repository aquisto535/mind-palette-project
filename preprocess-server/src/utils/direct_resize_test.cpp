#include <opencv2/opencv.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static cv::Mat resizeKeepingAspectRatio(const cv::Mat& input, int targetSize, int interpolation) {
    const double scale = std::min(
        static_cast<double>(targetSize) / input.cols,
        static_cast<double>(targetSize) / input.rows
    );
    const int newWidth = static_cast<int>(input.cols * scale);
    const int newHeight = static_cast<int>(input.rows * scale);

    cv::Mat resized;
    cv::resize(input, resized, cv::Size(newWidth, newHeight), 0, 0, interpolation);
    return resized;
}

static double benchmarkResize(const cv::Mat& input, int targetSize, int interpolation, int iterations) {
    cv::Mat output;
    resizeKeepingAspectRatio(input, targetSize, interpolation);

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        output = resizeKeepingAspectRatio(input, targetSize, interpolation);
    }
    const auto end = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration<double, std::milli>(end - start).count();
    return elapsed / iterations;
}

static cv::Rect findDarkContentRoi(const cv::Mat& image) {
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    cv::Mat mask;
    cv::threshold(gray, mask, 220, 255, cv::THRESH_BINARY_INV);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty()) {
        return cv::Rect(0, 0, image.cols, image.rows);
    }

    cv::Rect roi;
    for (const auto& contour : contours) {
        const cv::Rect rect = cv::boundingRect(contour);
        roi = roi.empty() ? rect : (roi | rect);
    }

    const int padding = 20;
    roi.x = std::max(0, roi.x - padding);
    roi.y = std::max(0, roi.y - padding);
    roi.width = std::min(image.cols - roi.x, roi.width + padding * 2);
    roi.height = std::min(image.rows - roi.y, roi.height + padding * 2);
    return roi;
}

static cv::Rect centerCropInRoi(const cv::Rect& roi, const cv::Size& bounds, int cropSize) {
    const int width = std::min(cropSize, roi.width);
    const int height = std::min(cropSize, roi.height);
    int x = roi.x + (roi.width - width) / 2;
    int y = roi.y + (roi.height - height) / 2;

    x = std::max(0, std::min(x, bounds.width - width));
    y = std::max(0, std::min(y, bounds.height - height));
    return cv::Rect(x, y, width, height);
}

static void saveZoomCrop(const std::string& path, const cv::Mat& image, const cv::Rect& crop) {
    cv::Mat zoom;
    cv::resize(image(crop), zoom, cv::Size(crop.width * 6, crop.height * 6), 0, 0, cv::INTER_NEAREST);
    cv::imwrite(path, zoom);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: direct_resize_test <image_path> [output_dir]\n";
        return 1;
    }

    const std::string imagePath = argv[1];
    const std::string outputDir = argc >= 3
        ? argv[2]
        : "docs/pipeline-stages/images/preprocess/direct-test";

    fs::create_directories(outputDir);

    const cv::Mat original = cv::imread(imagePath);
    if (original.empty()) {
        std::cerr << "Failed to load: " << imagePath << "\n";
        return 1;
    }

    const std::vector<int> targetSizes = {512, 768, 1024};
    const int iterations = 50;
    std::ofstream csv(outputDir + "/resize-test.csv");

    std::cout << "[input] " << imagePath << " -> "
              << original.cols << "x" << original.rows << "\n";
    std::cout << "[benchmark] average of " << iterations << " iterations\n";
    std::cout << "targetSize,INTER_AREA_ms,INTER_LINEAR_ms,output_size\n";
    csv << "targetSize,INTER_AREA_ms,INTER_LINEAR_ms,output_size\n";

    for (const int targetSize : targetSizes) {
        const cv::Mat area = resizeKeepingAspectRatio(original, targetSize, cv::INTER_AREA);
        const cv::Mat linear = resizeKeepingAspectRatio(original, targetSize, cv::INTER_LINEAR);
        const double areaMs = benchmarkResize(original, targetSize, cv::INTER_AREA, iterations);
        const double linearMs = benchmarkResize(original, targetSize, cv::INTER_LINEAR, iterations);

        const std::string base = outputDir + "/resize-" + std::to_string(targetSize);
        cv::imwrite(base + "-area.png", area);
        cv::imwrite(base + "-linear.png", linear);

        cv::Mat diff;
        cv::absdiff(area, linear, diff);
        cv::imwrite(base + "-absdiff.png", diff);

        const cv::Rect roi = findDarkContentRoi(linear);
        const cv::Rect crop = centerCropInRoi(roi, linear.size(), 96);
        saveZoomCrop(base + "-area-edge-crop-6x.png", area, crop);
        saveZoomCrop(base + "-linear-edge-crop-6x.png", linear, crop);

        std::cout << targetSize << ","
                  << std::fixed << std::setprecision(4) << areaMs << ","
                  << std::fixed << std::setprecision(4) << linearMs << ","
                  << area.cols << "x" << area.rows << "\n";
        csv << targetSize << ","
            << std::fixed << std::setprecision(4) << areaMs << ","
            << std::fixed << std::setprecision(4) << linearMs << ","
            << area.cols << "x" << area.rows << "\n";
    }

    std::cout << "[output] " << outputDir << "\n";
    return 0;
}
