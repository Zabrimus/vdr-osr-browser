#ifndef LOGGER_H
#define LOGGER_H

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/fmt/bin_to_hex.h"

#define CONSOLE_TRACE(...)     if (logger.level() == spdlog::level::trace) logger.current()->trace(__VA_ARGS__)
#define CONSOLE_DEBUG(...)     logger.current()->debug(__VA_ARGS__)
#define CONSOLE_INFO(...)      logger.current()->info(__VA_ARGS__)
#define CONSOLE_ERROR(...)     logger.current()->error(__VA_ARGS__)
#define CONSOLE_CRITICAL(...)  logger.current()->critical(__VA_ARGS__)

class Logger {
private:
    std::shared_ptr<spdlog::logger> _logger;
    bool _switchedToFile = false;

public:
    Logger();
    ~Logger();

    // Must be called before setting the desired level
    void switchToFileLogger(std::string filename);

    void set_level(spdlog::level::level_enum level) {
        _logger->set_level(level);
    }

    bool isTraceEnabled() {
        return _logger->level() == spdlog::level::trace;
    }

    bool isDebugEnabled() {
        return _logger->level() == spdlog::level::debug;
    }

    spdlog::level::level_enum level() {
        return _logger->level();
    }

    inline std::shared_ptr<spdlog::logger> current() {
        return _logger;
    }

    inline bool switchedToFile() {
        return this->_switchedToFile;
    }
};

extern Logger logger;

#endif // LOGGER_H
