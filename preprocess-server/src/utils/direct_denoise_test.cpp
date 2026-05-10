#include <opencv2/opencv.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct CaseConfig {
    std::string name;
    int gaussianSize;
    int medianSize;
};

struct CaseResult {
    CaseConfig config;
    cv::Size outputSize;
    double meanAbsDiffFromBaseline;
    int binaryPixels;
    int smallComponents;
};

static cv::Mat resizeKeepingAspectRatio(const cv::Mat& input, int targetSize) {
    const double scale = std::min(
        static_cast<double>(targetSize) / input.cols,
        static_cast<double>(targetSize) / input.rows
    );
    const int newWidth = static_cast<int>(input.cols * scale);
    const int newHeight = static_cast<int>(input.rows * scale);

    cv::Mat resized;
    cv::resize(input, resized, cv::Size(newWidth, newHeight), 0, 0, cv::INTER_AREA);
    return resized;
}

static cv::Mat applyDenoise(const cv::Mat& input, int gaussianSize, int medianSize) {
    cv::Mat result = input.clone();
    cv::GaussianBlur(result, result, cv::Size(gaussianSize, gaussianSize), 0);
    if (medianSize > 0) {
        cv::medianBlur(result, result, medianSize);
    }
    return result;
}

static cv::Mat binarizeLikeStage03b(const cv::Mat& input) {
    cv::Mat gray;
    if (input.channels() > 1) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input;
    }

    cv::Mat binary;
    cv::adaptiveThreshold(
        gray,
        binary,
        255,
        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
        cv::THRESH_BINARY_INV,
        11,
        2
    );
    return binary;
}

static int countSmallComponents(const cv::Mat& binary) {
    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int count = cv::connectedComponentsWithStats(binary, labels, stats, centroids, 8, CV_32S);

    int smallComponents = 0;
    for (int label = 1; label < count; ++label) {
        const int area = stats.at<int>(label, cv::CC_STAT_AREA);
        if (area >= 1 && area <= 8) {
            ++smallComponents;
        }
    }
    return smallComponents;
}

static cv::Rect findDarkContentRoi(const cv::Mat& image) {
    cv::Mat gray;
    if (image.channels() > 1) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image;
    }

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
        std::cerr << "Usage: direct_denoise_test <image_path> [output_dir]\n";
        return 1;
    }

    const std::string imagePath = argv[1];
    const std::string outputDir = argc >= 3
        ? argv[2]
        : "shared_volume/preprocess-denoise-test";

    fs::create_directories(outputDir);

    const cv::Mat original = cv::imread(imagePath);
    if (original.empty()) {
        std::cerr << "Failed to load: " << imagePath << "\n";
        return 1;
    }

    const cv::Mat resized = resizeKeepingAspectRatio(original, 768);
    cv::imwrite(outputDir + "/00-resize-768-input.png", resized);

    const std::vector<CaseConfig> cases = {
        {"gaussian-3", 3, 0},
        {"gaussian-5", 5, 0},
        {"gaussian-7", 7, 0},
        {"gaussian-3-median-3", 3, 3},
    };

    const cv::Mat baseline = applyDenoise(resized, 3, 0);
    const cv::Rect crop = centerCropInRoi(findDarkContentRoi(resized), resized.size(), 96);
    std::vector<CaseResult> results;

    for (const auto& testCase : cases) {
        const cv::Mat denoised = applyDenoise(resized, testCase.gaussianSize, testCase.medianSize);
        const cv::Mat binary = binarizeLikeStage03b(denoised);

        cv::Mat diff;
        cv::absdiff(baseline, denoised, diff);
        const cv::Scalar meanDiff = cv::mean(diff);
        const double meanAbsDiff = (meanDiff[0] + meanDiff[1] + meanDiff[2]) / 3.0;

        const std::string prefix = outputDir + "/" + testCase.name;
        cv::imwrite(prefix + "-denoised.png", denoised);
        cv::imwrite(prefix + "-binarized.png", binary);
        cv::imwrite(prefix + "-absdiff-from-gaussian-3.png", diff);
        saveZoomCrop(prefix + "-denoised-edge-crop-6x.png", denoised, crop);
        saveZoomCrop(prefix + "-binarized-edge-crop-6x.png", binary, crop);

        results.push_back({
            testCase,
            denoised.size(),
            meanAbsDiff,
            cv::countNonZero(binary),
            countSmallComponents(binary),
        });
    }

    std::ofstream errorLog(outputDir + "/gaussian-6-error.txt");
    try {
        const cv::Mat invalid = applyDenoise(resized, 6, 0);
        cv::imwrite(outputDir + "/gaussian-6-denoised.png", invalid);
        errorLog << "No exception occurred.\n";
    } catch (const cv::Exception& error) {
        errorLog << error.what() << "\n";
    }

    std::ofstream csv(outputDir + "/denoise-test.csv");
    csv << "case,gaussianSize,medianSize,meanAbsDiffFromGaussian3,binaryPixels,smallComponents,outputSize\n";
    for (const auto& result : results) {
        csv << result.config.name << ","
            << result.config.gaussianSize << ","
            << result.config.medianSize << ","
            << std::fixed << std::setprecision(4) << result.meanAbsDiffFromBaseline << ","
            << result.binaryPixels << ","
            << result.smallComponents << ","
            << result.outputSize.width << "x" << result.outputSize.height << "\n";
    }

    std::ofstream md(outputDir + "/denoise-test.md");
    md << "| case | Gaussian | Median | mean abs diff from Gaussian 3 | binary pixels | small components | note |\n";
    md << "|---|---:|---:|---:|---:|---:|---|\n";
    for (const auto& result : results) {
        md << "| " << result.config.name << " | "
           << result.config.gaussianSize << " | "
           << result.config.medianSize << " | "
           << std::fixed << std::setprecision(4) << result.meanAbsDiffFromBaseline << " | "
           << result.binaryPixels << " | "
           << result.smallComponents << " |  |\n";
    }

    std::cout << "[output] " << outputDir << "\n";
    return 0;
}
