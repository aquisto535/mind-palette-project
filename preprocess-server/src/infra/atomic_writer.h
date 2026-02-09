#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief AtomicFileWriter - Atomic write using .tmp → rename pattern
 * 
 * Ensures file integrity even if process crashes during write.
 * Pattern: write to .tmp file, then rename (atomic on most file systems).
 */
class AtomicFileWriter {
public:
    /**
     * @brief Atomically write an image to file
     * @param image OpenCV Mat to write
     * @param path Target file path
     * @return true if successful
     */
    static bool write(const cv::Mat& image, const std::string& path);
    
    /**
     * @brief Atomically write text/JSON to file
     * @param content String content to write
     * @param path Target file path
     * @return true if successful
     */
    static bool writeText(const std::string& content, const std::string& path);

private:
    static std::string getTempPath(const std::string& path);
    
    /**
     * @brief Save as PPM (fallback format)
     */
    static bool saveAsPPM(const cv::Mat& image, const std::string& path);
};
