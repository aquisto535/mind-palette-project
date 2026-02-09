#pragma once

#include "core/filter.h"

/**
 * @brief CannyFilter - Canny edge detection
 */
class CannyFilter : public IFilter {
public:
    explicit CannyFilter(double lowThreshold = 50, double highThreshold = 150);
    
    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "CannyFilter"; }

private:
    double lowThreshold_;
    double highThreshold_;
};

