#pragma once

#include "core/filter.h"

/**
 * @brief ColorValidationFilter - HSV 기반 컬러 이미지 조기 거부 필터.
 *
 * IFilter 구현체. FilterPipeline의 ResizeFilter 직후에 위치한다.
 *
 * ADR-033 Two-Step 로직:
 *   1. BGR → HSV 변환
 *   2. 밝고 비채색인 픽셀만 종이 배경으로 간주해 검사 제외
 *   3. 비배경 픽셀 모수 중 S >= satThreshold_ 및 chroma >= chromaThreshold_ 인 픽셀 비율 계산
 *   4. 비율이 colorPixelRatio_ 이상이면 ValidationException throw
 *
 * ADR-033: 비연필(컬러) 이미지 조기 필터링 전략 - Tier 1
 */
class ColorValidationFilter : public IFilter {
public:
    // 기본값 상수 — ADR-033 명시
    static constexpr int    kDefaultSatThreshold    = 50;   // S >= 50 → 컬러 픽셀
    static constexpr int    kDefaultValueThreshold  = 220;  // V > 220 이고 비채색 → 종이 배경(제외)
    static constexpr double kDefaultColorPixelRatio = 0.05; // 5% 이상 → reject
    static constexpr int    kDefaultChromaThreshold = 26;   // max(R,G,B)-min(R,G,B) >= 26 → 실제 색 번짐/채색 후보

    /**
     * @param satThreshold    HSV S채널 임계값 (0~255). 이 값 이상이 컬러 픽셀.
     * @param colorPixelRatio 컬러 픽셀 비율 임계값 (0.0~1.0). 이 값 이상이면 거부.
     * @param valueThreshold  HSV V채널 배경 임계값. 이 값 초과 픽셀은 검사에서 제외.
     */
    explicit ColorValidationFilter(
        int satThreshold       = kDefaultSatThreshold,
        double colorPixelRatio = kDefaultColorPixelRatio,
        int valueThreshold     = kDefaultValueThreshold,
        int chromaThreshold    = kDefaultChromaThreshold
    );

    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "ColorValidationFilter"; }

private:
    int    satThreshold_;
    double colorPixelRatio_;
    int    valueThreshold_;
    int    chromaThreshold_;

    /**
     * @brief BGR 이미지에서 종이 배경을 제외한 픽셀 중 컬러 픽셀 비율을 계산한다.
     * @param input BGR 3채널 이미지 (비어 있지 않음이 보장된 상태)
     * @return 비배경 픽셀 모수 대비 컬러 픽셀 비율 (0.0~1.0).
     *         비배경 픽셀이 0이면 0.0을 반환(종이만 있는 이미지는 통과).
     */
    double computeColorRatio(const cv::Mat& input) const;
};
