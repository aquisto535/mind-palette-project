#pragma once

#include "filter_pipeline.h"
#include "filters/resize_filter.h"
#include "filters/denoise_filter.h"
#include "filters/grayscale_filter.h"
#include "filters/canny_filter.h"
#include "filters/binarize_filter.h"
#include "filters/morphology_filter.h"
#include "filters/rgb_convert_filter.h"

/**
 * @brief PipelineFactory - Factory Pattern for pre-defined pipelines
 * 
 * Eliminates repetitive pipeline creation code.
 * Provides consistent pipeline configurations.
 */
class PipelineFactory {
public:
    /**
     * @brief Create basic preprocessing pipeline (Week 2)
     * Resize -> Denoise -> Grayscale
     */
    static FilterPipeline createPreprocessPipeline(int targetSize = 512);
    
    /**
     * @brief Create sketch analysis pipeline (Week 3)
     * Resize -> Denoise -> Grayscale -> Canny -> Morphology -> Binarize -> RGB
     */
    static FilterPipeline createSketchPipeline(int targetSize = 512);
    
    /**
     * @brief Create edge detection only pipeline
     * Grayscale -> Canny
     */
    static FilterPipeline createEdgeDetectionPipeline();
};
