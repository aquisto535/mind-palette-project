#include "infra/atomic_writer.h"
#include "utils/Logger.h"
#include <fstream>

bool AtomicFileWriter::write(const cv::Mat& image, const std::string& path, const std::string& requestId) 
{
    if (image.empty()) 
    {
        return false;
    }
    
    std::string tempPath = getTempPath(path); // 임시 파일 경로 생성
    
    try
    {
        // Ensure parent directory exists
        fs::path filePath(path);
        if (filePath.has_parent_path())
        {
            fs::create_directories(filePath.parent_path());
        }
        
        // Determine extension for encoding
        std::string ext = filePath.extension().string();
        if (ext.empty()) ext = ".jpg";
        
        // Try cv::imencode + binary write (more reliable than cv::imwrite)
        std::vector<uchar> buffer;
        try 
        {
            if (cv::imencode(ext, image, buffer)) // 이미지를 버퍼에 인코딩
            {
                // Write binary data to temp file
                std::ofstream ofs(tempPath, std::ios::binary);
                if (ofs && !buffer.empty())
                {
                    ofs.write(reinterpret_cast<const char*>(buffer.data()), buffer.size()); // 버퍼에 인코딩된 이미지 데이터를 파일에 쓴다
                    ofs.close();
                    if (ofs) 
                    {
                        // Atomic rename
                        fs::rename(tempPath, path); // 원본 파일로 원자적으로 이름 변경
                        return true;
                    }
                }
            }

        } 
        catch (const cv::Exception& e)
        {
            LOG_ERROR(requestId, "cv::imencode failed: {}", e.what());
        } 
        catch (...)
        {
            // Unknown encoding error
        }
        
        LOG_ERROR(requestId, "cv::imencode failed for path: {}", path);
        return false;
        
    } catch (const std::exception& e) {
        LOG_ERROR(requestId, "AtomicFileWriter error: {}", e.what());
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

bool AtomicFileWriter::atomicDelete(const std::string& path) {
    if (!fs::exists(path)) {
        return false;
    }

    std::string delPath = path + ".del";
    try {
        // Rename first (atomic on most file systems) — file appears gone to readers
        fs::rename(path, delPath);
        // Now remove the renamed file
        fs::remove(delPath);
        return true;
    } catch (const std::exception& e) {
        // If rename succeeded but remove failed, clean up .del
        try {
            if (fs::exists(delPath)) fs::remove(delPath);
        } catch (...) {}
        return false;
    }
}

std::string AtomicFileWriter::getTempPath(const std::string& path) {
    return path + ".tmp";
}

