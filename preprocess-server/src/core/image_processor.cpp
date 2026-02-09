#include "core/image_processor.h"
#include <iostream>

ImageProcessor::ImageProcessor() {
}

cv::Mat ImageProcessor::Load(const std::string& path) {
    cv::Mat img = cv::imread(path);
    if (img.empty()) {
        std::cerr << "Failed to load image: " << path << std::endl;
    }
    return img;
}

cv::Mat ImageProcessor::Preprocess(const cv::Mat& input) {
    if (input.empty()) {
        return cv::Mat();
    }
    
    cv::Mat processed;
    input.copyTo(processed);
    
    // Step 1: Resize to 512x512 with aspect ratio preservation (Letterbox)
    processed = ResizeKeepingAspectRatio(processed, kTargetSize);
    
    // Step 2: Noise reduction (GaussianBlur + medianBlur)
    cv::GaussianBlur(processed, processed, cv::Size(5, 5), 0);
    cv::medianBlur(processed, processed, 3);
    
    // Step 3: Convert to grayscale for edge detection
    cv::Mat grayscale;
    cv::cvtColor(processed, grayscale, cv::COLOR_BGR2GRAY);
    
    return grayscale;
}

bool ImageProcessor::Save(const cv::Mat& image, const std::string& path) {
    if (image.empty()) {
        return false;
    }
    return cv::imwrite(path, image);
}

// === Week 3: Advanced Preprocessing Implementation ===

cv::Mat ImageProcessor::RemoveBackground(const cv::Mat& input, int iterCount) {
    if (input.empty()) {
        return cv::Mat();
    }
    
    // Initialize mask: edge 10% as probable background, center as probable foreground
    cv::Mat mask = cv::Mat::zeros(input.size(), CV_8UC1);
    
    // Define margin (10% of image size)
    int marginX = input.cols / 10;
    int marginY = input.rows / 10;
    
    // Set center region as probable foreground (GC_PR_FGD = 3)
    cv::Rect foregroundRect(marginX, marginY, 
                            input.cols - 2 * marginX, 
                            input.rows - 2 * marginY);
    mask(foregroundRect).setTo(cv::GC_PR_FGD);
    
    // Temporary arrays for GrabCut
    cv::Mat bgdModel, fgdModel;
    
    // Apply GrabCut with mask initialization
    // Deep Dive Note: iterCount affects speed vs quality
    // - iterCount=1: ~20-30ms, lower quality
    // - iterCount=3: ~40-60ms, balanced (default)
    // - iterCount=5: ~80-100ms, higher quality
    cv::grabCut(input, mask, cv::Rect(), bgdModel, fgdModel, iterCount, cv::GC_INIT_WITH_MASK);
    
    // Convert GrabCut mask to binary mask
    // GC_FGD=1, GC_PR_FGD=3 -> 255 (foreground)
    // GC_BGD=0, GC_PR_BGD=2 -> 0 (background)
    cv::Mat result;
    cv::compare(mask, cv::GC_PR_FGD, result, cv::CMP_EQ);
    cv::Mat fgdMask;
    cv::compare(mask, cv::GC_FGD, fgdMask, cv::CMP_EQ);
    result = result | fgdMask;
    
    return result;
}

cv::Mat ImageProcessor::DetectEdges(const cv::Mat& grayscale, double lowThreshold, double highThreshold) {
    if (grayscale.empty()) {
        return cv::Mat();
    }
    
    // Ensure input is grayscale
    cv::Mat input = grayscale;
    if (grayscale.channels() > 1) {
        cv::cvtColor(grayscale, input, cv::COLOR_BGR2GRAY);
    }
    
    cv::Mat edges;
    // Canny edge detection
    // apertureSize=3 (Sobel kernel size)
    // L2gradient=true for more accurate gradient magnitude
    cv::Canny(input, edges, lowThreshold, highThreshold, 3, true);
    
    return edges;
}

cv::Mat ImageProcessor::EnhanceContours(const cv::Mat& binary, int kernelSize) {
    if (binary.empty()) {
        return cv::Mat();
    }
    
    // Create structuring element for morphology
    cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_RECT, 
        cv::Size(kernelSize, kernelSize)
    );
    
    cv::Mat result;
    // MORPH_CLOSE: Dilation followed by Erosion
    // Closes small gaps in contours while preserving shape
    cv::morphologyEx(binary, result, cv::MORPH_CLOSE, kernel);
    
    return result;
}

cv::Mat ImageProcessor::Binarize(const cv::Mat& grayscale) {
    if (grayscale.empty()) {
        return cv::Mat();
    }
    
    // Ensure input is grayscale
    cv::Mat input = grayscale;
    if (grayscale.channels() > 1) {
        cv::cvtColor(grayscale, input, cv::COLOR_BGR2GRAY);
    }
    
    cv::Mat result;
    // Adaptive thresholding: works better for varying lighting conditions
    // ADAPTIVE_THRESH_GAUSSIAN_C: weighted sum (Gaussian blur) of neighborhood
    // THRESH_BINARY_INV: dark objects on light background
    // blockSize=11: neighborhood size
    // C=2: constant subtracted from mean
    cv::adaptiveThreshold(input, result, 255, 
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C, 
                          cv::THRESH_BINARY_INV, 
                          11, 2);
    
    return result;
}

cv::Mat ImageProcessor::ResizeKeepingAspectRatio(const cv::Mat& input, int targetSize) {
    if (input.empty()) {
        return cv::Mat();
    }
    
    // Calculate scaling factor to fit within targetSize while preserving aspect ratio
    double scale = std::min(
        static_cast<double>(targetSize) / input.cols,
        static_cast<double>(targetSize) / input.rows
    );
    
    int newWidth = static_cast<int>(input.cols * scale);
    int newHeight = static_cast<int>(input.rows * scale);
    
    cv::Mat resized;
    cv::resize(input, resized, cv::Size(newWidth, newHeight));
    
    // Create canvas with black padding
    cv::Mat canvas(targetSize, targetSize, input.type(), cv::Scalar(0, 0, 0));
    
    // Center the resized image on the canvas
    int offsetX = (targetSize - newWidth) / 2;
    int offsetY = (targetSize - newHeight) / 2;
    resized.copyTo(canvas(cv::Rect(offsetX, offsetY, newWidth, newHeight)));
    
    return canvas;
}
