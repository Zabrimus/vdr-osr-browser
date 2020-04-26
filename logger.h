#ifndef LOGGER_H
#define LOGGER_H

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/fmt/bin_to_hex.h"

#define CONSOLE_TRACE(...)     logger.console()->trace(__VA_ARGS__);
#define CONSOLE_DEBUG(...)     logger.console()->debug(__VA_ARGS__);
#define CONSOLE_INFO(...)      logger.console()->info(__VA_ARGS__);
#define CONSOLE_INFO(...)      logger.console()->info(__VA_ARGS__);
#define CONSOLE_ERROR(...)     logger.console()->error(__VA_ARGS__);
#define CONSOLE_CRITICAL(...)  logger.console()->critical(__VA_ARGS__);

class Logger {
private:
    std::shared_ptr<spdlog::logger> logger_console;

public:
    Logger();
    ~Logger();

    inline std::shared_ptr<spdlog::logger> console() {
        return logger_console;
    }
};

extern Logger logger;

#endif // LOGGER_H
