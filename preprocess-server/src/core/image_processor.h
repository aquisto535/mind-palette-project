#pragma once

#include <string>
#include <opencv2/opencv.hpp>

class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor() = default;

    // Load image from path
    cv::Mat Load(const std::string& path);

    // Apply preprocessing steps: Resize -> Denoise -> Grayscale
    cv::Mat Preprocess(const cv::Mat& input);

    // Save image to path
    bool Save(const cv::Mat& image, const std::string& path);

// [DEAD CODE] Disabled for performance reasons
    // cv::Mat RemoveBackground(const cv::Mat& input, int iterCount = 3);

    // Canny edge detection
    // @param grayscale: Single-channel grayscale image
    // @param lowThreshold: Lower hysteresis threshold (default 50)
    // @param highThreshold: Upper hysteresis threshold (default 150)
    cv::Mat DetectEdges(const cv::Mat& grayscale, double lowThreshold = 50, double highThreshold = 150);

    // Morphological contour enhancement (MORPH_CLOSE)
    // @param binary: Single-channel binary image
    // @param kernelSize: Structuring element size (default 3)
    cv::Mat EnhanceContours(const cv::Mat& binary, int kernelSize = 3);

    // Adaptive thresholding for binarization
    // @param grayscale: Single-channel grayscale image
    cv::Mat Binarize(const cv::Mat& grayscale);

    // Resize image while preserving aspect ratio (Letterbox)
    // Fills empty space with black padding
    // @param input: Input image
    // @param targetSize: Target width/height (default 512)
    cv::Mat ResizeKeepingAspectRatio(const cv::Mat& input, int targetSize = 512);

    // Calculate ROI containing all significant contours
    cv::Rect GetContentROI(const cv::Mat& binary);

    // Crop image to ROI
    cv::Mat Crop(const cv::Mat& image, const cv::Rect& roi);

private:
    static constexpr int kTargetSize = 512;
};
