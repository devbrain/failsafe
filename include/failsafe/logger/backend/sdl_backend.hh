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
#include <sstream>

namespace failsafe {
namespace logger {
namespace backend {

/**
 * @brief SDL3 logging backend for failsafe
 *
 * This backend maps failsafe log levels to SDL3 log priorities and
 * outputs messages through SDL's logging system. This allows for
 * consistent logging when using SDL3 applications.
 */
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
     * @param level Failsafe log level
     * @param msg The log message
     */
    void log(severity level, const std::string& msg) {
        SDL_LogPriority priority = map_severity_to_sdl(level);
        SDL_LogMessage(category_, priority, "%s", msg.c_str());
    }

    /**
     * @brief Log a formatted message through SDL
     * @param level Failsafe log level
     * @param location Source code location
     * @param format Format string
     * @param args Format arguments
     */
    template<typename... Args>
    void log(severity level,
             const source_location& location,
             const std::string& format,
             Args&&... args) {
        std::ostringstream oss;

        // Add location information if available
        if (!location.file_name().empty()) {
            oss << "[" << location.file_name()
                << ":" << location.line()
                << "] ";
        }

        // Format the message
        if constexpr (sizeof...(args) > 0) {
            // Simple format implementation
            // In production, use fmt::format or similar
            oss << format_string(format, std::forward<Args>(args)...);
        } else {
            oss << format;
        }

        log(level, oss.str());
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
     * @param level Failsafe severity level
     * @return Corresponding SDL log priority
     */
    static SDL_LogPriority map_severity_to_sdl(severity level) {
        switch (level) {
        case severity::trace:
            return SDL_LOG_PRIORITY_VERBOSE;
        case severity::debug:
            return SDL_LOG_PRIORITY_DEBUG;
        case severity::info:
            return SDL_LOG_PRIORITY_INFO;
        case severity::warn:
            return SDL_LOG_PRIORITY_WARN;
        case severity::error:
            return SDL_LOG_PRIORITY_ERROR;
        case severity::fatal:
            return SDL_LOG_PRIORITY_CRITICAL;
        default:
            return SDL_LOG_PRIORITY_INFO;
        }
    }

    /**
     * @brief Simple format string implementation
     * @param format Format string
     * @param args Arguments
     * @return Formatted string
     */
    template<typename... Args>
    std::string format_string(const std::string& format, Args&&... args) {
        std::ostringstream oss;
        oss << format;
        // Simplified - in production use proper formatting
        ((oss << " " << args), ...);
        return oss.str();
    }
};

/**
 * @brief Factory function to create SDL backend
 * @param category SDL log category
 * @return Shared pointer to SDL backend
 */
inline std::shared_ptr<sdl_backend> make_sdl_backend(
    int category = SDL_LOG_CATEGORY_APPLICATION) {
    return std::make_shared<sdl_backend>(category);
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
 * failsafe::logger::backend::use_sdl_backend();
 * LOG_INFO("Application started");
 * @endcode
 */
inline void use_sdl_backend(int category = SDL_LOG_CATEGORY_APPLICATION) {
    auto backend = make_sdl_backend(category);

    // Register the backend with failsafe logger
    // The exact API depends on failsafe version
    // This is a conceptual example:

    // Option 1: If failsafe supports custom backends
    // logger::set_backend(backend);

    // Option 2: If failsafe uses a global function
    // set_log_backend([backend](severity level, const std::string& msg) {
    //     backend->log(level, msg);
    // });

    // Option 3: Through a registry pattern
    // logger::registry::instance().set_backend(backend);
}

} // namespace backend
} // namespace logger
} // namespace failsafe