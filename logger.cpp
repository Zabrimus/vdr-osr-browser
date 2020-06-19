#include "logger.h"
#include "spdlog/cfg/env.h"

Logger::Logger() {
    spdlog::cfg::load_env_levels();
    _logger =  spdlog::stdout_color_mt("vdrosrbrowser_c");

    _logger->set_level(spdlog::level::trace);
}

Logger::~Logger() {
}

void Logger::switchToFileLogger(std::string filename) {
    auto max_size = 1048576 * 5;
    auto max_files = 3;
    _logger = spdlog::rotating_logger_mt("vdrosrbrowser_f", filename, max_size, max_files);
}

Logger logger = Logger();