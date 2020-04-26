#include "logger.h"
#include "spdlog/cfg/env.h"

Logger::Logger() {
    spdlog::cfg::load_env_levels();
    logger_console =  spdlog::stdout_color_mt("vdrosrbrowser");

    logger_console->set_level(spdlog::level::trace);
}

Logger::~Logger() {
}

Logger logger = Logger();