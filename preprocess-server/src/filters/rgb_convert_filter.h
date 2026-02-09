#pragma once

#include "core/filter.h"

/**
 * @brief RgbConvertFilter - Convert grayscale/binary to RGB 3-channel
 * 
 * Required for EfficientNet-B2 compatibility (expects 3-channel input)
 */
class RgbConvertFilter : public IFilter {
public:
    RgbConvertFilter() = default;
    
    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "RgbConvertFilter"; }
};

