#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <memory>

/**
 * @brief IFilter - Strategy Pattern Interface
 * 
 * All image filters implement this interface.
 * Enables OCP (Open-Closed Principle): add new filters without modifying existing code.
 * 
 * Reference: refactoring.guru/design-patterns/strategy
 */
class IFilter {
public:
    virtual ~IFilter() = default;
    
    /**
     * @brief Apply filter to input image
     * @param input Input cv::Mat image
     * @return Processed cv::Mat image
     */
    virtual cv::Mat apply(const cv::Mat& input) const = 0;
    
    /**
     * @brief Get filter name (for logging/debugging)
     * @return Filter name as string
     */
    virtual std::string name() const = 0;
};

// Type alias for filter pointer
using FilterPtr = std::unique_ptr<IFilter>;
