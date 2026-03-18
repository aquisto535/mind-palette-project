#pragma once

#include <string>
#include <opencv2/opencv.hpp>

class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor() = default;

    // Load image from path
    cv::Mat Load(const std::string& path, const std::string& requestId = "SYSTEM");

    // Apply preprocessing steps: Resize -> Denoise -> Grayscale
    cv::Mat Preprocess(const cv::Mat& input, const std::string& requestId = "SYSTEM");

    // Save image to path
    bool Save(const cv::Mat& image, const std::string& path, const std::string& requestId = "SYSTEM");

    // Canny edge detection
    cv::Mat DetectEdges(const cv::Mat& grayscale, double lowThreshold = 50, double highThreshold = 150);

    // Morphological contour enhancement (MORPH_CLOSE)
    cv::Mat EnhanceContours(const cv::Mat& binary, int kernelSize = 3);

    // Adaptive thresholding for binarization
    cv::Mat Binarize(const cv::Mat& grayscale);

    // Resize image while preserving aspect ratio (Letterbox)
    cv::Mat ResizeKeepingAspectRatio(const cv::Mat& input, int targetSize = 512);

    // Calculate ROI containing all significant contours
    cv::Rect GetContentROI(const cv::Mat& binary);

    // Crop image to ROI
    cv::Mat Crop(const cv::Mat& image, const cv::Rect& roi);

private:
    static constexpr int kTargetSize = 512;

    // Helper methods for Preprocess pipeline stages (Refactored from monolithic Preprocess)
    cv::Mat NormalizeGrayscale(const cv::Mat& input);
    cv::Mat ApplyAdaptiveBinarization(const cv::Mat& input);
    
    struct ResizeResult {
        cv::Mat canvas;
        int x_offset;
        int y_offset;
        int new_w;
        int new_h;
    };
    ResizeResult ApplyLetterboxWithMetrics(const cv::Mat& input, int targetSize, int interpolation, uint8_t padValue = 255);
    cv::Mat GenerateDistanceMap(const cv::Mat& binary);
};
