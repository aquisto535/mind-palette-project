#include "filter_pipeline.h"
#include <iostream>

FilterPipeline& FilterPipeline::add(FilterPtr filter) {
    if (filter) {
        filters_.push_back(std::move(filter));
    }
    return *this;
}

cv::Mat FilterPipeline::execute(const cv::Mat& input) const {
    if (input.empty() || filters_.empty()) {
        return input.clone();
    }
    
    cv::Mat result = input.clone();
    
    for (const auto& filter : filters_) {
        result = filter->apply(result);
        
        if (result.empty()) {
            std::cerr << "Filter '" << filter->name() << "' returned empty result" << std::endl;
            return cv::Mat();
        }
    }
    
    return result;
}

void FilterPipeline::clear() {
    filters_.clear();
}
