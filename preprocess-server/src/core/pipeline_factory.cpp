#include "core/pipeline_factory.h"

FilterPipeline PipelineFactory::createPreprocessPipeline(int targetSize) {
    FilterPipeline pipeline;
    
    pipeline.add(std::make_unique<ResizeFilter>(targetSize))
            .add(std::make_unique<DenoiseFilter>())
            .add(std::make_unique<GrayscaleFilter>());
    
    return pipeline;
}



FilterPipeline PipelineFactory::createHybridPipeline() {
    FilterPipeline pipeline;
    
    // Step 1: Initial Resize (768) to optimize AdaptiveThreshold and FindContours speed (< 100ms target)
    // withPadding=false to avoid black bars interfering with ROI detection
    pipeline.add(std::make_unique<ResizeFilter>(768, false));

    // Step 2: [ADR-033 Tier 1] Color Validation — 컬러 이미지 조기 거부
    // Two-Step: (1) V>220 이고 비채색인 픽셀만 종이 배경으로 제외,
    //           (2) 비배경 픽셀 중 S>=50 && chroma>=26 비율 ≥5% → 422.
    pipeline.add(std::make_unique<ColorValidationFilter>(
        ColorValidationFilter::kDefaultSatThreshold,
        ColorValidationFilter::kDefaultColorPixelRatio,
        ColorValidationFilter::kDefaultValueThreshold));

    // Step 3: Denoise (Gaussian Only)
    // Gaussian: 5x5, Median: 0 (Disabled)
    pipeline.add(std::make_unique<DenoiseFilter>(5, 0));
    
    // Step 3-6: Hybrid Processing (Binarize -> SmartCrop -> Merge)
    pipeline.add(std::make_unique<HybridPreprocessFilter>());
    
    return pipeline;
}

