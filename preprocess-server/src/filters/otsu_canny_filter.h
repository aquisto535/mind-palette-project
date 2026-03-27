#pragma once

#include "core/filter.h"

/**
 * @brief OtsuCannyFilter - Canny edge detection with Otsu-based auto thresholding
 *
 * Computes optimal threshold via Otsu's method, then applies Canny:
 *   low  = (1 - sigma) * otsu
 *   high = (1 + sigma) * otsu
 * BGR input is converted to grayscale first.
 * Output is single-channel edge map (CV_8UC1).
 */
class OtsuCannyFilter : public IFilter {
public:
    explicit OtsuCannyFilter(double sigma = 0.5);

    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "OtsuCannyFilter"; }

private:
    double sigma_;
};
