#pragma once

#include "core/filter.h"

/**
 * @brief ClaheFilter - CLAHE histogram equalization for uneven lighting compensation
 *
 * Applies Contrast Limited Adaptive Histogram Equalization (CLAHE).
 * BGR input is converted to grayscale first.
 * Output is always single-channel (CV_8UC1).
 */
class ClaheFilter : public IFilter {
public:
    explicit ClaheFilter(double clipLimit = 2.0, int tileGridSize = 8);

    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "ClaheFilter"; }

private:
    double clipLimit_;
    int tileGridSize_;
};
