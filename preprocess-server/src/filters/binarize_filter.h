#pragma once

#include "core/filter.h"

/**
 * @brief BinarizeFilter - Adaptive thresholding for binarization
 */
class BinarizeFilter : public IFilter {
public:
    explicit BinarizeFilter(int blockSize = 11, int c = 2);
    
    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "BinarizeFilter"; }

private:
    int blockSize_;
    int c_;
};

