#include "otsu_canny_filter.h"

OtsuCannyFilter::OtsuCannyFilter(double sigma) : sigma_(sigma) {}

cv::Mat OtsuCannyFilter::apply(const cv::Mat& input) const {
    if (input.empty()) {
        return cv::Mat();
    }

    cv::Mat gray;
    if (input.channels() == 3) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input.clone();
    }

    // Otsu's method: threshold() with THRESH_OTSU returns the optimal threshold value
    cv::Mat dummy;
    double otsu = cv::threshold(gray, dummy, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    double low  = (1.0 - sigma_) * otsu;
    double high = (1.0 + sigma_) * otsu;

    cv::Mat edges;
    cv::Canny(gray, edges, low, high);
    return edges;
}
