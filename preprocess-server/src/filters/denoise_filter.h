#pragma once

#include "core/filter.h"

/**
 * @brief DenoiseFilter - Gaussian + Median blur for noise reduction
 * 
 * Applies GaussianBlur for high-frequency noise and medianBlur for salt-and-pepper noise.
 */
class DenoiseFilter : public IFilter {
public:
    explicit DenoiseFilter(int gaussianSize = 5, int medianSize = 3);
    
    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "DenoiseFilter"; }

private:
    int gaussianSize_;
    int medianSize_;
};

