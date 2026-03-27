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
    
    // Step 2: Denoise (Gaussian Only)
    // Gaussian: 5x5, Median: 0 (Disabled)
    pipeline.add(std::make_unique<DenoiseFilter>(5, 0));
    
    // Step 3-6: Hybrid Processing (Binarize -> SmartCrop -> Merge)
    pipeline.add(std::make_unique<HybridPreprocessFilter>());
    
    return pipeline;
}

