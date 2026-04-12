#include <opencv2/opencv.hpp>
#include "filters/resize_filter.h"
#include "filters/denoise_filter.h"
#include "filters/binarize_filter.h"
#include "filters/morphology_filter.h"
#include "filters/hybrid_preprocess_filter.h"
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// Replicated from HybridPreprocessFilter::GetContentROI (private)
// Used only for ROI bounding-box visualization.
static cv::Rect GetContentROI(const cv::Mat& binary) {
    if (binary.empty()) return cv::Rect(0, 0, 0, 0);

    cv::Mat morph;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::morphologyEx(binary, morph, cv::MORPH_CLOSE, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(morph, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) return cv::Rect(0, 0, binary.cols, binary.rows);

    double totalArea = static_cast<double>(binary.cols) * binary.rows;
    std::vector<std::pair<double, cv::Rect>> validRects;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area < totalArea * 0.001) continue;
        validRects.push_back({area, cv::boundingRect(contour)});
    }

    if (validRects.empty()) return cv::Rect(0, 0, binary.cols, binary.rows);

    auto dominantIt = std::max_element(validRects.begin(), validRects.end(),
        [](const std::pair<double, cv::Rect>& a, const std::pair<double, cv::Rect>& b) {
            return a.first < b.first;
        });
    cv::Rect roi = dominantIt->second;

    const int padding = 10;
    roi.x = std::max(0, roi.x - padding);
    roi.y = std::max(0, roi.y - padding);
    roi.width  = std::min(binary.cols - roi.x, roi.width  + 2 * padding);
    roi.height = std::min(binary.rows - roi.y, roi.height + 2 * padding);
    return roi;
}

static bool save(const std::string& path, const cv::Mat& img) {
    if (cv::imwrite(path, img)) {
        std::cout << "  Saved: " << path << "\n";
        return true;
    }
    std::cerr << "  ERROR: Failed to save " << path << "\n";
    return false;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: visualize_pipeline <image_path> [output_dir]\n";
        std::cerr << "  output_dir defaults to docs/pipeline-stages/images/preprocess\n";
        return 1;
    }

    const std::string imagePath = argv[1];
    const std::string outputDir = (argc >= 3)
        ? argv[2]
        : "docs/pipeline-stages/images/preprocess";

    fs::create_directories(outputDir);

    // ------------------------------------------------------------------
    // Stage 00: Original
    // ------------------------------------------------------------------
    cv::Mat original = cv::imread(imagePath);
    if (original.empty()) {
        std::cerr << "Failed to load: " << imagePath << "\n";
        return 1;
    }
    std::cout << "[00] Original: " << original.cols << "x" << original.rows << "\n";
    save(outputDir + "/00-original.png", original);

    // ------------------------------------------------------------------
    // Stage 01: ResizeFilter (fit within 768, no padding)
    // ------------------------------------------------------------------
    std::cout << "[01] ResizeFilter(768)...\n";
    cv::Mat resized = ResizeFilter(768, false).apply(original);
    std::cout << "     -> " << resized.cols << "x" << resized.rows << "\n";
    save(outputDir + "/01-resize-768.png", resized);

    // ------------------------------------------------------------------
    // Stage 02: DenoiseFilter (Gaussian 5x5, Median disabled)
    // ------------------------------------------------------------------
    std::cout << "[02] DenoiseFilter(gaussian=5, median=0)...\n";
    cv::Mat denoised = DenoiseFilter(5, 0).apply(resized);
    save(outputDir + "/02-denoised.png", denoised);

    // ------------------------------------------------------------------
    // Stage 03a: Grayscale conversion
    // ------------------------------------------------------------------
    std::cout << "[03a] Grayscale...\n";
    cv::Mat gray;
    cv::cvtColor(denoised, gray, cv::COLOR_BGR2GRAY);
    save(outputDir + "/03a-grayscale.png", gray);

    // ------------------------------------------------------------------
    // Stage 03b: Adaptive Binarization (BINARY_INV, blockSize=11, C=2)
    // ------------------------------------------------------------------
    std::cout << "[03b] BinarizeFilter(blockSize=11, C=2)...\n";
    cv::Mat binary = BinarizeFilter(11, 2).apply(gray);
    save(outputDir + "/03b-binarized.png", binary);

    // ------------------------------------------------------------------
    // Stage 03c: Morphology MORPH_CLOSE (kernel 3x3)
    // ------------------------------------------------------------------
    std::cout << "[03c] MorphologyFilter(MORPH_CLOSE, kernel=3)...\n";
    cv::Mat morphed = MorphologyFilter(3, cv::MORPH_CLOSE).apply(binary);
    save(outputDir + "/03c-morphology.png", morphed);

    // ------------------------------------------------------------------
    // Stage 03d: ROI detection — draw bounding box on denoised image
    // ------------------------------------------------------------------
    std::cout << "[03d] ROI detection...\n";
    cv::Rect roi = GetContentROI(morphed);
    cv::Mat roiViz = denoised.clone();
    cv::rectangle(roiViz, roi, cv::Scalar(0, 0, 255), 3);
    std::cout << "     -> ROI: " << roi.x << "," << roi.y
              << " " << roi.width << "x" << roi.height << "\n";
    save(outputDir + "/03d-roi-detected.png", roiViz);

    // ------------------------------------------------------------------
    // Stage 03e: Letterbox crop -> 512x512 grayscale canvas
    // ------------------------------------------------------------------
    std::cout << "[03e] Letterbox crop to 512x512...\n";
    {
        const int kTargetSize = 512;
        cv::Rect safeRoi = roi & cv::Rect(0, 0, gray.cols, gray.rows);
        cv::Mat roiGray = gray(safeRoi).clone();

        float scale = std::min(
            static_cast<float>(kTargetSize) / safeRoi.width,
            static_cast<float>(kTargetSize) / safeRoi.height);
        int new_w = static_cast<int>(safeRoi.width  * scale);
        int new_h = static_cast<int>(safeRoi.height * scale);

        cv::Mat scaledGray;
        cv::resize(roiGray, scaledGray, cv::Size(new_w, new_h), 0, 0, cv::INTER_AREA);

        cv::Mat canvas(kTargetSize, kTargetSize, CV_8UC1, cv::Scalar(255));
        int x_off = (kTargetSize - new_w) / 2;
        int y_off = (kTargetSize - new_h) / 2;
        scaledGray.copyTo(canvas(cv::Rect(x_off, y_off, new_w, new_h)));
        save(outputDir + "/03e-letterbox-512.png", canvas);
    }

    // ------------------------------------------------------------------
    // Stage 04: Full HybridPreprocessFilter -> channel split
    //
    // ConstructChannels merges as {ch_R, ch_G, ch_B}, so after cv::split:
    //   channels[0] = ch_R  (grayscale / pressure)
    //   channels[1] = ch_G  (inverted binary / shape)
    //   channels[2] = ch_B  (distance map / stroke width)
    // ------------------------------------------------------------------
    std::cout << "[04] HybridPreprocessFilter...\n";
    cv::Mat hybrid = HybridPreprocessFilter().apply(denoised);
    if (hybrid.empty()) {
        std::cerr << "HybridPreprocessFilter returned empty result.\n";
        return 1;
    }

    std::vector<cv::Mat> channels;
    cv::split(hybrid, channels);

    save(outputDir + "/04-ch-R-gray.png",     channels[0]);
    save(outputDir + "/04-ch-G-binary.png",   channels[1]);
    save(outputDir + "/04-ch-B-distance.png", channels[2]);
    save(outputDir + "/04-final-3ch.png",     hybrid);

    std::cout << "\nDone. " << outputDir << " 에 총 12개 이미지 저장됨.\n";
    return 0;
}
