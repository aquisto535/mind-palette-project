#pragma once

#include "filter.h"
#include <vector>

/**
 * @brief FilterPipeline - Composite Pattern for chaining filters
 * 
 * Allows dynamic composition of filters at runtime.
 * Filters are executed in the order they were added.
 */
class FilterPipeline {
public:
    FilterPipeline() = default;
    
    // Disable copy (unique_ptr semantics)
    FilterPipeline(const FilterPipeline&) = delete;
    FilterPipeline& operator=(const FilterPipeline&) = delete;
    
    // Enable move
    FilterPipeline(FilterPipeline&&) = default;
    FilterPipeline& operator=(FilterPipeline&&) = default;
    
    /**
     * @brief Add a filter to the pipeline
     * @param filter Filter to add (ownership transferred)
     * @return Reference to this pipeline (for chaining)
     */
    FilterPipeline& add(FilterPtr filter);
    
    /**
     * @brief Execute all filters in order
     * @param input Input image
     * @return Processed image
     */
    cv::Mat execute(const cv::Mat& input) const;
    
    /**
     * @brief Clear all filters from the pipeline
     */
    void clear();
    
    /**
     * @brief Get number of filters in the pipeline
     */
    size_t size() const { return filters_.size(); }
    
    /**
     * @brief Check if pipeline is empty
     */
    bool empty() const { return filters_.empty(); }

private:
    std::vector<FilterPtr> filters_;
};
