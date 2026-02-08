#pragma once

#include "../filter.h"

/**
 * @brief GrayscaleFilter - Convert BGR to Grayscale
 */
class GrayscaleFilter : public IFilter {
public:
    GrayscaleFilter() = default;
    
    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "GrayscaleFilter"; }
};
