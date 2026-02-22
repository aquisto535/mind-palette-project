#pragma once

#include "core/filter_pipeline.h"
#include "filters/resize_filter.h"
#include "filters/denoise_filter.h"
#include "filters/grayscale_filter.h"
#include "filters/grayscale_filter.h"
#include "filters/binarize_filter.h"
#include "filters/morphology_filter.h"
#include "filters/rgb_convert_filter.h"
#include "filters/rgb_convert_filter.h"
#include "filters/invert_filter.h"
#include "filters/hybrid_preprocess_filter.h"

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
     * @brief Create Hybrid 3-Channel Strategy Pipeline (Phase 3 Completed)
     * Resize(1024) -> Denoise(Gaussian) -> HybridPreprocess(SmartCrop+Merge)
     */
    static FilterPipeline createHybridPipeline();
};

