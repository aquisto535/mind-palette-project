#pragma once

#include "core/filter.h"

/**
 * @brief ColorValidationFilter - HSV 채도 분석으로 컬러 이미지를 조기 거부한다.
 *
 * IFilter 구현체. FilterPipeline의 ResizeFilter 직후에 위치한다.
 *
 * 로직:
 *   1. BGR 이미지를 HSV 색공간으로 변환
 *   2. S(채도) 채널에서 satThreshold_ 초과 픽셀 비율을 계산
 *   3. 비율이 colorPixelRatio_ 이상이면 ValidationException을 throw
 *
 * ADR-033: 비연필(컬러) 이미지 조기 필터링 전략 - Tier 1
 */
class ColorValidationFilter : public IFilter {
public:
    // 기본값 상수 — 매직 넘버 제거
    static constexpr int    kDefaultSatThreshold    = 30;
    static constexpr double kDefaultColorPixelRatio = 0.05;

    /**
     * @param satThreshold    HSV S채널 임계값 (0~255). 이 값 초과 픽셀을 컬러로 판단.
     * @param colorPixelRatio 컬러 픽셀 비율 임계값 (0.0~1.0). 이 값 이상이면 거부.
     */
    explicit ColorValidationFilter(
        int satThreshold       = kDefaultSatThreshold,
        double colorPixelRatio = kDefaultColorPixelRatio
    );

    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "ColorValidationFilter"; }

private:
    int    satThreshold_;
    double colorPixelRatio_;

    /**
     * @brief BGR 이미지에서 채도 임계값을 초과하는 픽셀 비율을 계산한다.
     * @param input BGR 3채널 이미지 (비어 있지 않음이 보장된 상태)
     * @return 전체 픽셀 대비 컬러 픽셀 비율 (0.0~1.0)
     */
    double computeColorRatio(const cv::Mat& input) const;
};
