#include "utils/quality_metrics.h"

double QualityMetrics::computePSNR(const cv::Mat& original, const cv::Mat& processed) {
    if (original.empty() || processed.empty()) {
        return -1.0;
    }
    if (original.size() != processed.size() || original.type() != processed.type()) {
        return -1.0;
    }
    return cv::PSNR(original, processed);
}

double QualityMetrics::computeSSIM(const cv::Mat& original, const cv::Mat& processed) {
    if (original.empty() || processed.empty()) {
        return -1.0;
    }
    if (original.size() != processed.size() || original.type() != processed.type()) {
        return -1.0;
    }

    cv::Mat img1, img2;
    original.convertTo(img1, CV_64F);
    processed.convertTo(img2, CV_64F);

    const double C1 = 6.5025;   // (0.01 * 255)^2
    const double C2 = 58.5225;  // (0.03 * 255)^2

    cv::Mat mu1, mu2;
    cv::GaussianBlur(img1, mu1, cv::Size(11, 11), 1.5);
    cv::GaussianBlur(img2, mu2, cv::Size(11, 11), 1.5);

    cv::Mat mu1_sq  = mu1.mul(mu1);
    cv::Mat mu2_sq  = mu2.mul(mu2);
    cv::Mat mu1_mu2 = mu1.mul(mu2);

    cv::Mat sigma1_sq, sigma2_sq, sigma12;
    cv::GaussianBlur(img1.mul(img1), sigma1_sq, cv::Size(11, 11), 1.5);
    sigma1_sq -= mu1_sq;

    cv::GaussianBlur(img2.mul(img2), sigma2_sq, cv::Size(11, 11), 1.5);
    sigma2_sq -= mu2_sq;

    cv::GaussianBlur(img1.mul(img2), sigma12, cv::Size(11, 11), 1.5);
    sigma12 -= mu1_mu2;

    cv::Mat ssim_map;
    cv::Mat numerator   = (2.0 * mu1_mu2 + C1).mul(2.0 * sigma12 + C2);
    cv::Mat denominator = (mu1_sq + mu2_sq + C1).mul(sigma1_sq + sigma2_sq + C2);
    cv::divide(numerator, denominator, ssim_map);

    cv::Scalar mean = cv::mean(ssim_map);
    return mean[0];
}
