#pragma once

/**
 * @file sdl_backend.hh
 * @brief SDL3 backend for failsafe logging
 *
 * This backend integrates failsafe logging with SDL3's logging system,
 * allowing failsafe log messages to be output through SDL_LogMessage.
 */

#include <SDL3/SDL_log.h>
#include <failsafe/logger.hh>
#include <string>

/**
 * @brief SDL3 logging backend for failsafe
 *
 * This backend maps failsafe log levels to SDL3 log priorities and
 * outputs messages through SDL's logging system. This allows for
 * consistent logging when using SDL3 applications.
 */
namespace failsafe::logger::backends {

class sdl_backend {
public:
    /**
     * @brief Construct SDL backend with category
     * @param category SDL log category (default: SDL_LOG_CATEGORY_APPLICATION)
     */
    explicit sdl_backend(int category = SDL_LOG_CATEGORY_APPLICATION)
        : category_(category) {}

    /**
     * @brief Log a message through SDL
     * @param level Failsafe log level (LOGGER_LEVEL_* constant)
     * @param category Logger category
     * @param file Source file name
     * @param line Source line number
     * @param message The formatted log message
     */
    void operator()(int level,
                    const char* category,
                    const char* file,
                    int line,
                    const std::string& message) const {
        const SDL_LogPriority priority = map_level_to_sdl(level);

        if (file && *file) {
            SDL_LogMessage(category_, priority, "[%s] %s:%d - %s",
                           category ? category : "",
                           file,
                           line,
                           message.c_str());
        } else {
            SDL_LogMessage(category_, priority, "[%s] %s",
                           category ? category : "",
                           message.c_str());
        }
    }

    /**
     * @brief Set the SDL log category
     * @param category New SDL log category
     */
    void set_category(int category) {
        category_ = category;
    }

    /**
     * @brief Get the current SDL log category
     * @return Current category
     */
    int get_category() const {
        return category_;
    }

private:
    int category_;

    /**
     * @brief Map failsafe severity to SDL log priority
     * @param level Failsafe log level (LOGGER_LEVEL_* constant)
     * @return Corresponding SDL log priority
     */
    static SDL_LogPriority map_level_to_sdl(int level) {
        switch (level) {
        case LOGGER_LEVEL_TRACE:
            return SDL_LOG_PRIORITY_VERBOSE;
        case LOGGER_LEVEL_DEBUG:
            return SDL_LOG_PRIORITY_DEBUG;
        case LOGGER_LEVEL_INFO:
            return SDL_LOG_PRIORITY_INFO;
        case LOGGER_LEVEL_WARN:
            return SDL_LOG_PRIORITY_WARN;
        case LOGGER_LEVEL_ERROR:
            return SDL_LOG_PRIORITY_ERROR;
        case LOGGER_LEVEL_FATAL:
            return SDL_LOG_PRIORITY_CRITICAL;
        default:
            return SDL_LOG_PRIORITY_INFO;
        }
    }
};

/**
 * @brief Factory function to create SDL backend
 * @param category SDL log category
 * @return LoggerBackend containing the configured SDL backend
 */
inline LoggerBackend make_sdl_backend(
    int category = SDL_LOG_CATEGORY_APPLICATION) {
    return sdl_backend(category);
}

/**
 * @brief Configure failsafe to use SDL backend
 * @param category SDL log category
 *
 * This is a convenience function that sets up failsafe to use
 * SDL's logging system as its backend.
 *
 * Example usage:
 * @code
 * failsafe::logger::backends::use_sdl_backend();
 * LOG_INFO("Application started");
 * @endcode
 */
inline void use_sdl_backend(int category = SDL_LOG_CATEGORY_APPLICATION) {
    set_backend(make_sdl_backend(category));
}

} // namespace failsafe::logger::backends
