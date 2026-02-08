#pragma once

#include "../filter.h"

/**
 * @brief MorphologyFilter - Morphological operations (MORPH_CLOSE)
 */
class MorphologyFilter : public IFilter {
public:
    explicit MorphologyFilter(int kernelSize = 3, int operation = cv::MORPH_CLOSE);
    
    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "MorphologyFilter"; }

private:
    int kernelSize_;
    int operation_;
};
