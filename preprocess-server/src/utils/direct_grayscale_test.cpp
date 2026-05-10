#include <opencv2/opencv.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

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

static cv::Mat denoiseLikeStage02(const cv::Mat& input) {
    cv::Mat denoised;
    cv::GaussianBlur(input, denoised, cv::Size(3, 3), 0);
    return denoised;
}

static cv::Mat adaptiveThresholdLikeStage03b(const cv::Mat& input) {
    cv::Mat binary;
    cv::adaptiveThreshold(
        input,
        binary,
        255,
        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
        cv::THRESH_BINARY_INV,
        11,
        2
    );
    return binary;
}

static int countNonZeroAbsDiff(const cv::Mat& a, const cv::Mat& b) {
    cv::Mat diff;
    cv::absdiff(a, b, diff);
    return cv::countNonZero(diff);
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
        std::cerr << "Usage: direct_grayscale_test <image_path> [output_dir]\n";
        return 1;
    }

    const std::string imagePath = argv[1];
    const std::string outputDir = argc >= 3
        ? argv[2]
        : "shared_volume/preprocess-grayscale-test";

    fs::create_directories(outputDir);

    const cv::Mat original = cv::imread(imagePath);
    if (original.empty()) {
        std::cerr << "Failed to load: " << imagePath << "\n";
        return 1;
    }

    const cv::Mat resized = resizeKeepingAspectRatio(original, 768);
    const cv::Mat denoised = denoiseLikeStage02(resized);

    cv::Mat weightedGray;
    cv::cvtColor(denoised, weightedGray, cv::COLOR_BGR2GRAY);

    cv::Mat greenChannel;
    cv::extractChannel(denoised, greenChannel, 1);

    cv::Mat grayVsGreenAbsDiff;
    cv::absdiff(weightedGray, greenChannel, grayVsGreenAbsDiff);

    const cv::Mat binaryFromWeightedGray = adaptiveThresholdLikeStage03b(weightedGray);
    const cv::Mat binaryFromGreenChannel = adaptiveThresholdLikeStage03b(greenChannel);
    cv::Mat binaryAbsDiff;
    cv::absdiff(binaryFromWeightedGray, binaryFromGreenChannel, binaryAbsDiff);

    cv::imwrite(outputDir + "/00-denoised-input.png", denoised);
    cv::imwrite(outputDir + "/01-bgr2gray-weighted.png", weightedGray);
    cv::imwrite(outputDir + "/02-green-channel.png", greenChannel);
    cv::imwrite(outputDir + "/03-gray-vs-green-absdiff.png", grayVsGreenAbsDiff);
    cv::imwrite(outputDir + "/04-binarized-from-bgr2gray.png", binaryFromWeightedGray);
    cv::imwrite(outputDir + "/05-binarized-from-green-channel.png", binaryFromGreenChannel);
    cv::imwrite(outputDir + "/06-binarized-absdiff.png", binaryAbsDiff);

    const cv::Rect crop = centerCropInRoi(findDarkContentRoi(denoised), denoised.size(), 96);
    saveZoomCrop(outputDir + "/01-bgr2gray-weighted-edge-crop-6x.png", weightedGray, crop);
    saveZoomCrop(outputDir + "/02-green-channel-edge-crop-6x.png", greenChannel, crop);
    saveZoomCrop(outputDir + "/04-binarized-from-bgr2gray-edge-crop-6x.png", binaryFromWeightedGray, crop);
    saveZoomCrop(outputDir + "/05-binarized-from-green-channel-edge-crop-6x.png", binaryFromGreenChannel, crop);

    std::ofstream errorLog(outputDir + "/adaptive-threshold-3ch-error.txt");
    try {
        const cv::Mat invalidBinary = adaptiveThresholdLikeStage03b(denoised);
        cv::imwrite(outputDir + "/invalid-binarized-from-3ch.png", invalidBinary);
        errorLog << "No exception occurred.\n";
    } catch (const cv::Exception& error) {
        errorLog << error.what() << "\n";
    }

    const cv::Scalar meanGray = cv::mean(weightedGray);
    const cv::Scalar meanGreen = cv::mean(greenChannel);
    const cv::Scalar meanDiff = cv::mean(grayVsGreenAbsDiff);

    std::ofstream csv(outputDir + "/grayscale-test.csv");
    csv << "metric,value\n";
    csv << "grayType," << weightedGray.type() << "\n";
    csv << "grayChannels," << weightedGray.channels() << "\n";
    csv << "greenType," << greenChannel.type() << "\n";
    csv << "greenChannels," << greenChannel.channels() << "\n";
    csv << "meanBgr2Gray," << std::fixed << std::setprecision(4) << meanGray[0] << "\n";
    csv << "meanGreenChannel," << std::fixed << std::setprecision(4) << meanGreen[0] << "\n";
    csv << "meanAbsDiffGrayVsGreen," << std::fixed << std::setprecision(4) << meanDiff[0] << "\n";
    csv << "nonZeroDiffGrayVsGreen," << countNonZeroAbsDiff(weightedGray, greenChannel) << "\n";
    csv << "binaryPixelsFromBgr2Gray," << cv::countNonZero(binaryFromWeightedGray) << "\n";
    csv << "binaryPixelsFromGreenChannel," << cv::countNonZero(binaryFromGreenChannel) << "\n";
    csv << "binaryDiffPixels," << cv::countNonZero(binaryAbsDiff) << "\n";
    csv << "outputSize," << weightedGray.cols << "x" << weightedGray.rows << "\n";

    std::ofstream md(outputDir + "/grayscale-test.md");
    md << "| metric | value |\n";
    md << "|---|---:|\n";
    md << "| grayType | " << weightedGray.type() << " |\n";
    md << "| grayChannels | " << weightedGray.channels() << " |\n";
    md << "| meanAbsDiffGrayVsGreen | " << std::fixed << std::setprecision(4) << meanDiff[0] << " |\n";
    md << "| binaryPixelsFromBgr2Gray | " << cv::countNonZero(binaryFromWeightedGray) << " |\n";
    md << "| binaryPixelsFromGreenChannel | " << cv::countNonZero(binaryFromGreenChannel) << " |\n";
    md << "| binaryDiffPixels | " << cv::countNonZero(binaryAbsDiff) << " |\n";

    std::cout << "[output] " << outputDir << "\n";
    return 0;
}
