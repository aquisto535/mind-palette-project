#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

class Logger {
public:
    static void init(const std::string& log_dir = "logs");
    static std::shared_ptr<spdlog::logger> get();

private:
    static std::shared_ptr<spdlog::logger> logger_;
};

#define LOG_DEBUG(req_id, ...) spdlog::debug("[{}] " __VA_ARGS__, req_id)
#define LOG_INFO(req_id, ...)  spdlog::info("[{}] " __VA_ARGS__, req_id)
#define LOG_WARN(req_id, ...)  spdlog::warn("[{}] " __VA_ARGS__, req_id)
#define LOG_ERROR(req_id, ...) spdlog::error("[{}] " __VA_ARGS__, req_id)
