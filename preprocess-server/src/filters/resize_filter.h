#pragma once

#include "../filter.h"

/**
 * @brief ResizeFilter - Letterbox resize with aspect ratio preservation
 * 
 * Resizes image to target size while preserving aspect ratio.
 * Empty space is filled with black padding.
 */
class ResizeFilter : public IFilter {
public:
    explicit ResizeFilter(int targetSize = 512);
    
    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "ResizeFilter"; }

private:
    int targetSize_;
};
