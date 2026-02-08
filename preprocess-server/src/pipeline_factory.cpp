#include "pipeline_factory.h"

FilterPipeline PipelineFactory::createPreprocessPipeline(int targetSize) {
    FilterPipeline pipeline;
    
    pipeline.add(std::make_unique<ResizeFilter>(targetSize))
            .add(std::make_unique<DenoiseFilter>())
            .add(std::make_unique<GrayscaleFilter>());
    
    return pipeline;
}

FilterPipeline PipelineFactory::createSketchPipeline(int targetSize) {
    FilterPipeline pipeline;
    
    pipeline.add(std::make_unique<ResizeFilter>(targetSize))
            .add(std::make_unique<DenoiseFilter>())
            .add(std::make_unique<GrayscaleFilter>())
            .add(std::make_unique<CannyFilter>(50, 150))
            .add(std::make_unique<MorphologyFilter>(3))
            .add(std::make_unique<BinarizeFilter>())
            .add(std::make_unique<RgbConvertFilter>());
    
    return pipeline;
}

FilterPipeline PipelineFactory::createEdgeDetectionPipeline() {
    FilterPipeline pipeline;
    
    pipeline.add(std::make_unique<GrayscaleFilter>())
            .add(std::make_unique<CannyFilter>(50, 150));
    
    return pipeline;
}
