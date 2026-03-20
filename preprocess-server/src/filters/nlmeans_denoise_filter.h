#pragma once

#include "core/filter.h"

/**
 * @brief NlMeansDenoiseFilter - Edge-preserving denoising via Non-Local Means
 *
 * Uses cv::fastNlMeansDenoising (grayscale) or
 * cv::fastNlMeansDenoisingColored (BGR) depending on input channels.
 * Preserves edges better than Gaussian/median blur.
 */
class NlMeansDenoiseFilter : public IFilter {
public:
    explicit NlMeansDenoiseFilter(float h = 10.0f, int templateWindowSize = 7, int searchWindowSize = 21);

    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "NlMeansDenoiseFilter"; }

private:
    float h_;
    int templateWindowSize_;
    int searchWindowSize_;
};
