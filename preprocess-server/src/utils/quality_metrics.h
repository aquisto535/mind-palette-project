#pragma once

#include <opencv2/opencv.hpp>

/**
 * @brief QualityMetrics - Image quality measurement utilities
 *
 * Provides PSNR and SSIM computation for evaluating filter output quality.
 * Returns negative values on invalid (empty/mismatched) inputs.
 */
class QualityMetrics {
public:
    /**
     * Compute Peak Signal-to-Noise Ratio between two images.
     * @return PSNR in dB (>50 means near-identical), or -1.0 on error.
     */
    static double computePSNR(const cv::Mat& original, const cv::Mat& processed);

    /**
     * Compute Structural Similarity Index between two images.
     * @return SSIM in [0, 1] (1.0 = identical), or -1.0 on error.
     */
    static double computeSSIM(const cv::Mat& original, const cv::Mat& processed);
};
