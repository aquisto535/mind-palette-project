#include "analysis/pressure_analyzer.h"

PressureResult PressureAnalyzer::analyze(const cv::Mat& input) const {
    PressureResult result;
    result.pressureScore  = -1.0;
    result.meanIntensity  = -1.0;
    result.stdIntensity   = -1.0;
    result.histogram.assign(256, 0.0f);

    if (input.empty() || input.channels() < 3) {
        return result;
    }

    // BGR에서 채널 분리 → R채널 (index 2)
    std::vector<cv::Mat> channels;
    cv::split(input, channels);
    const cv::Mat& rChannel = channels[2];

    // 히스토그램 계산
    int histSize   = 256;
    float range[]  = {0.0f, 256.0f};
    const float* ranges[] = {range};
    cv::Mat hist;
    cv::calcHist(&rChannel, 1, nullptr, cv::Mat(), hist, 1, &histSize, ranges);

    result.histogram.resize(256);
    for (int i = 0; i < 256; ++i) {
        result.histogram[i] = hist.at<float>(i);
    }

    // 평균 / 표준편차
    cv::Scalar mean, stddev;
    cv::meanStdDev(rChannel, mean, stddev);
    result.meanIntensity = mean[0];
    result.stdIntensity  = stddev[0];

    // pressureScore = 1.0 - (meanIntensity / 255.0)
    result.pressureScore = 1.0 - (result.meanIntensity / 255.0);

    return result;
}
