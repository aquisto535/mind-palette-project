#include "infra/atomic_writer.h"
#include <fstream>
#include <iostream>

bool AtomicFileWriter::write(const cv::Mat& image, const std::string& path) {
    if (image.empty()) {
        return false;
    }
    
    std::string tempPath = getTempPath(path);
    
    try {
        // Ensure parent directory exists
        fs::path filePath(path);
        if (filePath.has_parent_path()) {
            fs::create_directories(filePath.parent_path());
        }
        
        // Determine extension for encoding
        std::string ext = filePath.extension().string();
        if (ext.empty()) ext = ".jpg";
        
        // Try cv::imencode + binary write (more reliable than cv::imwrite)
        std::vector<uchar> buffer;
        try {
            if (cv::imencode(ext, image, buffer)) {
                // Write binary data to temp file
                std::ofstream ofs(tempPath, std::ios::binary);
                if (ofs && !buffer.empty()) {
                    ofs.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                    ofs.close();
                    if (ofs) {
                        // Atomic rename
                        fs::rename(tempPath, path);
                        return true;
                    }
                }
            }
        } catch (const cv::Exception& e) {
            std::cerr << "cv::imencode failed: " << e.what() << std::endl;
        } catch (...) {
            // Ignore to try fallback
        }
        
        std::cerr << "Failed to write image (imencode). Attempting PPM fallback..." << std::endl;
        
        // Fallback to PPM
        std::string ppmPath = path;
        if (ppmPath.find(".ppm") == std::string::npos) {
            ppmPath = fs::path(path).replace_extension(".ppm").string();
        }
        std::string tempPpmPath = getTempPath(ppmPath);
        
        if (saveAsPPM(image, tempPpmPath)) {
            fs::rename(tempPpmPath, ppmPath);
            std::cout << "Fallback success: Saved as " << ppmPath << std::endl;
            try { if (fs::exists(tempPath)) fs::remove(tempPath); } catch (...) {}
            return true;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "AtomicFileWriter error: " << e.what() << std::endl;
        try { if (fs::exists(tempPath)) fs::remove(tempPath); } catch (...) {}
        return false;
    }
}

bool AtomicFileWriter::writeText(const std::string& content, const std::string& path) {
    // ... (unchanged)
    std::string tempPath = getTempPath(path);
    
    try {
        fs::path filePath(path);
        if (filePath.has_parent_path()) {
            fs::create_directories(filePath.parent_path());
        }
        
        std::ofstream ofs(tempPath);
        if (!ofs) return false;
        
        ofs << content;
        ofs.close();
        
        if (!ofs) return false;
        
        fs::rename(tempPath, path);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "AtomicFileWriter error: " << e.what() << std::endl;
        try { if (fs::exists(tempPath)) fs::remove(tempPath); } catch (...) {}
        return false;
    }
}

std::string AtomicFileWriter::getTempPath(const std::string& path) {
    return path + ".tmp";
}

bool AtomicFileWriter::saveAsPPM(const cv::Mat& image, const std::string& path) {
    std::ofstream ofs(path);
    if (!ofs) return false;
    
    ofs << "P3\n" << image.cols << " " << image.rows << "\n255\n";
    
    cv::Mat rgb;
    if (image.channels() == 1) cv::cvtColor(image, rgb, cv::COLOR_GRAY2RGB);
    else if (image.channels() == 4) cv::cvtColor(image, rgb, cv::COLOR_BGRA2RGB);
    else cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);

    for(int y=0; y<rgb.rows; ++y) {
        for(int x=0; x<rgb.cols; ++x) {
            cv::Vec3b p = rgb.at<cv::Vec3b>(y,x);
            ofs << (int)p[0] << " " << (int)p[1] << " " << (int)p[2] << " ";
        }
        ofs << "\n";
    }
    return true;
}

