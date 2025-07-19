/**
 * @file poco_backend.hh
 * @brief POCO logging framework integration backend
 * 
 * @details
 * This backend integrates the failsafe logger with the POCO C++ Libraries
 * logging framework. It provides:
 * - Bidirectional severity level mapping
 * - Automatic message routing to POCO loggers
 * - Category-based logger selection
 * - Proper source location propagation
 * 
 * The integration allows you to use failsafe's logging API while leveraging
 * POCO's existing logging infrastructure, channels, and formatters.
 * 
 * @example
 * @code
 * // Configure POCO logging first
 * Poco::AutoPtr<Poco::ConsoleChannel> console(new Poco::ConsoleChannel);
 * Poco::Logger::root().setChannel(console);
 * 
 * // Set up failsafe logger with POCO backend
 * logger::set_backend(make_poco_backend());
 * 
 * // All logs now go through POCO
 * LOG_INFO("MyCategory", "Hello from failsafe through POCO!");
 * @endcode
 */
#pragma once
#include <failsafe/logger.hh>
#include "Poco/Logger.h"
#include "Poco/Message.h"

/**
 * @namespace failsafe::logger::backends
 * @brief Logger backend implementations
 */
namespace failsafe::logger::backends {
    /**
     * @brief Convert failsafe log level to POCO priority
     * 
     * Maps integer log levels from failsafe to POCO's Message::Priority enum.
     * 
     * @param level Failsafe log level (LOGGER_LEVEL_* constant)
     * @return Corresponding POCO Message::Priority value
     */
    inline Poco::Message::Priority level_to_poco_priority(int level) {
        switch (level) {
            case LOGGER_LEVEL_TRACE: return Poco::Message::PRIO_TRACE;
            case LOGGER_LEVEL_DEBUG: return Poco::Message::PRIO_DEBUG;
            case LOGGER_LEVEL_INFO: return Poco::Message::PRIO_INFORMATION;
            case LOGGER_LEVEL_WARN: return Poco::Message::PRIO_WARNING;
            case LOGGER_LEVEL_ERROR: return Poco::Message::PRIO_ERROR;
            case LOGGER_LEVEL_FATAL: return Poco::Message::PRIO_FATAL;
            default: return Poco::Message::PRIO_INFORMATION;
        }
    }

    /**
     * @brief Convert POCO priority to failsafe log level
     * 
     * Maps POCO's Message::Priority enum values to failsafe integer log levels.
     * CRITICAL is mapped to FATAL, and NOTICE is mapped to INFO.
     * 
     * @param priority POCO Message::Priority value
     * @return Corresponding failsafe log level
     */
    inline int poco_priority_to_level(Poco::Message::Priority priority) {
        switch (priority) {
            case Poco::Message::PRIO_TRACE: return LOGGER_LEVEL_TRACE;
            case Poco::Message::PRIO_DEBUG: return LOGGER_LEVEL_DEBUG;
            case Poco::Message::PRIO_INFORMATION: return LOGGER_LEVEL_INFO;
            case Poco::Message::PRIO_WARNING: return LOGGER_LEVEL_WARN;
            case Poco::Message::PRIO_ERROR: return LOGGER_LEVEL_ERROR;
            case Poco::Message::PRIO_FATAL: return LOGGER_LEVEL_FATAL;
            case Poco::Message::PRIO_CRITICAL: return LOGGER_LEVEL_FATAL; // Map CRITICAL to FATAL
            case Poco::Message::PRIO_NOTICE: return LOGGER_LEVEL_INFO; // Map NOTICE to INFO
            default: return LOGGER_LEVEL_INFO;
        }
    }

    /**
     * @brief POCO backend implementation
     * 
     * @details
     * Routes log messages from failsafe to POCO's logging system.
     * Uses the category parameter to select the appropriate POCO logger,
     * and includes source location information in the POCO Message.
     * 
     * The function checks if the logger is enabled for the given priority
     * level before constructing and logging the message.
     * 
     * @param level Log level (LOGGER_LEVEL_* constants)
     * @param category Logger category name (used to get POCO logger)
     * @param file Source file name
     * @param line Source line number
     * @param message The formatted log message
     */
    inline void poco_backend(int level, const char* category,
                             const char* file, int line,
                             const std::string& message) {
        Poco::Logger& logger = Poco::Logger::get(category);

        if (logger.is(level_to_poco_priority(level))) {
            logger.log(Poco::Message(
                category,
                message,
                level_to_poco_priority(level),
                file,
                line
            ));
        }
    }

    /**
     * @brief Factory function to create a POCO backend
     * 
     * Creates a LoggerBackend that routes all log messages through
     * the POCO logging framework.
     * 
     * @return LoggerBackend configured for POCO integration
     * 
     * @note Ensure POCO logging is properly configured before using this backend.
     * The backend will use POCO's existing logger hierarchy and channels.
     * 
     * @example
     * @code
     * // Set up POCO logging infrastructure
     * Poco::AutoPtr<Poco::FileChannel> fileChannel(new Poco::FileChannel);
     * fileChannel->setProperty("path", "application.log");
     * fileChannel->setProperty("rotation", "2 M");
     * 
     * Poco::AutoPtr<Poco::PatternFormatter> formatter(new Poco::PatternFormatter);
     * formatter->setProperty("pattern", "%Y-%m-%d %H:%M:%S [%p] %s: %t");
     * 
     * Poco::AutoPtr<Poco::FormattingChannel> formattingChannel(
     *     new Poco::FormattingChannel(formatter, fileChannel));
     * 
     * Poco::Logger::root().setChannel(formattingChannel);
     * 
     * // Now use with failsafe
     * logger::set_backend(make_poco_backend());
     * @endcode
     */
    inline LoggerBackend make_poco_backend() {
        return poco_backend;
    }
}
