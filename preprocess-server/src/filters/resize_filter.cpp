#include "resize_filter.h"

ResizeFilter::ResizeFilter(int targetSize) : targetSize_(targetSize) {}

cv::Mat ResizeFilter::apply(const cv::Mat& input) const {
    if (input.empty()) {
        return cv::Mat();
    }
    
    // Calculate scaling factor to fit within targetSize while preserving aspect ratio
    double scale = std::min(
        static_cast<double>(targetSize_) / input.cols,
        static_cast<double>(targetSize_) / input.rows
    );
    
    int newWidth = static_cast<int>(input.cols * scale);
    int newHeight = static_cast<int>(input.rows * scale);
    
    // INTER_AREA: 축소 시 안티앨리어싱 효과로 품질 우수
    // INTER_CUBIC: 확대 시 부드러운 보간으로 선명도 유지
    int interpolation = (scale < 1.0) ? cv::INTER_AREA : cv::INTER_CUBIC;

    cv::Mat resized;
    cv::resize(input, resized, cv::Size(newWidth, newHeight), 0, 0, interpolation);
    
    // Create canvas with black padding
    cv::Mat canvas(targetSize_, targetSize_, input.type(), cv::Scalar(0, 0, 0));
    
    // Center the resized image on the canvas
    int offsetX = (targetSize_ - newWidth) / 2;
    int offsetY = (targetSize_ - newHeight) / 2;
    resized.copyTo(canvas(cv::Rect(offsetX, offsetY, newWidth, newHeight)));
    
    return canvas;
}
