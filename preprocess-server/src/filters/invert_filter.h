#ifndef INVERT_FILTER_H
#define INVERT_FILTER_H

#include "core/filter.h"

class InvertFilter : public IFilter {
public:
    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "InvertFilter"; }
};

#endif // INVERT_FILTER_H

