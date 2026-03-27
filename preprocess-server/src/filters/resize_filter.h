#pragma once

#include "core/filter.h"

/**
 * @brief ResizeFilter - Letterbox resize with aspect ratio preservation
 * 
 * Resizes image to target size while preserving aspect ratio.
 * Empty space is filled with black padding.
 */
class ResizeFilter : public IFilter {
public:
    explicit ResizeFilter(int targetSize, bool withPadding = true);
    
    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "ResizeFilter"; }

private:
    int targetSize_;
    bool withPadding_;
};
