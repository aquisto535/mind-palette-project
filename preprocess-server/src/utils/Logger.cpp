#include "Logger.h"
#include <filesystem>
#include <vector>

std::shared_ptr<spdlog::logger> Logger::logger_;

void Logger::init(const std::string& log_dir) {
    if (!std::filesystem::exists(log_dir)) {
        std::filesystem::create_directories(log_dir);
    }

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_dir + "/preprocess.log", 1024 * 1024 * 10, 3);
    file_sink->set_level(spdlog::level::info);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");

    std::vector<spdlog::sink_ptr> sinks { console_sink, file_sink };
    logger_ = std::make_shared<spdlog::logger>("preprocess", sinks.begin(), sinks.end());
    
    spdlog::register_logger(logger_);
    spdlog::set_default_logger(logger_);
    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_on(spdlog::level::info);
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!logger_) {
        // Fallback if not initialized
        init();
    }
    return logger_;
}
