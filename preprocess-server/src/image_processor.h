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

private:
    const int kTargetSize = 512;
};
