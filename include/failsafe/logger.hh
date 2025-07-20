/**
 * @file logger.hh
 * @brief Thread-safe logging system with configurable backends and compile-time filtering
 * 
 * @details
 * The failsafe logger provides a flexible, thread-safe logging system with the following features:
 * - Multiple log levels (TRACE, DEBUG, INFO, WARN, ERROR, FATAL)
 * - Lazy evaluation by default - arguments are only evaluated if the log level is enabled
 * - Compile-time level filtering for zero-overhead when disabled
 * - Runtime level filtering
 * - Pluggable backend system
 * - Thread-safe by default
 * - Variadic message building with type-safe formatting
 * - Category-based logging
 * - Conditional logging
 * 
 * @note All logging macros use lazy evaluation by default. This means expensive operations
 * in log arguments are only executed when the log level is enabled, providing automatic
 * performance optimization without any special syntax.
 * 
 * @example
 * @code
 * // Basic logging - arguments are lazily evaluated
 * LOG_INFO("Starting application");
 * LOG_ERROR("Failed to connect:", error_code, "retrying in", delay, "seconds");
 * 
 * // Expensive operations are only called if DEBUG is enabled
 * LOG_DEBUG("Query result:", expensive_database_query());
 * 
 * // Category-based logging - also lazy by default
 * LOG_CAT_DEBUG("network", "Sending packet:", packet_id);
 * 
 * // Conditional logging
 * LOG_IF(verbose_mode, LOGGER_LEVEL_TRACE, "Detailed state:", state);
 * 
 * // Custom backend
 * logger::set_backend([](int level, const char* cat, const char* file, 
 *                       int line, const std::string& msg) {
 *     // Custom logging implementation
 * });
 * @endcode
 */
#pragma once

#include <sstream>
#include <utility>
#include <functional>
#include <atomic>
#include <string>
#include <iostream>
#include <mutex>

#include <failsafe/detail/string_utils.hh>
#include <failsafe/detail/location_format.hh>

/**
 * @defgroup LogLevels Logging Levels
 * @{
 */

/** @brief Most detailed logging level for tracing execution flow */
#define LOGGER_LEVEL_TRACE 0

/** @brief Debug messages for development and troubleshooting */
#define LOGGER_LEVEL_DEBUG 1

/** @brief Informational messages about normal operation */
#define LOGGER_LEVEL_INFO  2

/** @brief Warning messages for potentially problematic situations */
#define LOGGER_LEVEL_WARN  3

/** @brief Error messages for recoverable errors */
#define LOGGER_LEVEL_ERROR 4

/** @brief Fatal error messages for unrecoverable errors */
#define LOGGER_LEVEL_FATAL 5

/** @} */ // end of LogLevels group

/**
 * @brief Minimum compile-time log level
 * 
 * Messages below this level are completely removed at compile time.
 * Default is TRACE level (all levels enabled). Can be overridden by defining before including this header.
 * For production builds, consider setting this to LOGGER_LEVEL_INFO or higher to remove debug/trace logs.
 */
#ifndef LOGGER_MIN_LEVEL
#define LOGGER_MIN_LEVEL LOGGER_LEVEL_TRACE
#endif

/** @internal Stringification helper macro */
#define LOGGER_STRINGIFY(x) #x

/** @internal Stringification helper macro */
#define LOGGER_TOSTRING(x) LOGGER_STRINGIFY(x)

/**
 * @brief Default logging category
 * 
 * Used when no category is specified. Default is "Application".
 * Can be overridden by defining before including this header.
 */
#ifndef LOGGER_DEFAULT_CATEGORY
    #define LOGGER_DEFAULT_CATEGORY Application
#endif

/** @internal Create string from category macro */
#define LOGGER_DEFAULT_CATEGORY_STR LOGGER_TOSTRING(LOGGER_DEFAULT_CATEGORY)

/**
 * @namespace failsafe::logger
 * @brief Logger subsystem providing flexible, thread-safe logging
 */
namespace failsafe::logger {
    
    /**
     * @namespace failsafe::logger::internal
     * @brief Internal implementation details (not part of public API)
     */
    namespace internal {
        /**
         * @brief Convert log level to string representation
         * @param level The log level (LOGGER_LEVEL_*)
         * @return String representation of the level
         */
        inline constexpr const char* level_to_string(int level) {
            switch (level) {
                case LOGGER_LEVEL_TRACE: return "TRACE";
                case LOGGER_LEVEL_DEBUG: return "DEBUG";
                case LOGGER_LEVEL_INFO: return "INFO ";
                case LOGGER_LEVEL_WARN: return "WARN ";
                case LOGGER_LEVEL_ERROR: return "ERROR";
                case LOGGER_LEVEL_FATAL: return "FATAL";
                default: return "UNKNOWN";
            }
        }

        /**
         * @brief Default logging backend that writes to std::cerr
         * 
         * Thread-safe implementation that formats and outputs log messages to stderr.
         * 
         * @param level Log level
         * @param category Log category
         * @param file Source file name
         * @param line Source line number
         * @param message The formatted log message
         */
        inline void default_cerr_backend(int level, const char* category,
                                         const char* file, int line,
                                         const std::string& message) {
            static std::mutex cerr_mutex;
            std::lock_guard <std::mutex> lock(cerr_mutex);

            std::cerr << "[" << level_to_string(level) << "] "
                << "[" << category << "] "
                << ::failsafe::detail::format_location(file, line) << " - "
                << message << std::endl;
        }
    }

    /**
     * @brief Logger backend function type
     * 
     * Backends receive all log information and are responsible for
     * formatting and outputting the log messages.
     * 
     * @param level Log level (LOGGER_LEVEL_*)
     * @param category Log category string
     * @param file Source file name
     * @param line Source line number
     * @param message The formatted message string
     */
    using LoggerBackend = std::function <void(
        int level, // Log level
        const char* category, // Logger category
        const char* file, // Source file
        int line, // Line number
        const std::string& message // Concatenated message
    )>;

    /**
     * @brief Logger configuration structure
     * 
     * Holds the global logger state including backend, minimum level,
     * and enabled status.
     */
    struct LoggerConfig {
        /** @brief Current logging backend */
        LoggerBackend backend = internal::default_cerr_backend;
        
        /** @brief Minimum runtime log level (atomic for thread safety) */
        std::atomic <int> min_level{LOGGER_MIN_LEVEL};
        
        /** @brief Whether logging is enabled (atomic for thread safety) */
        std::atomic <bool> enabled{true};
    };

    /**
     * @brief Get global logger configuration
     * @return Reference to the global logger configuration
     */
    inline LoggerConfig& get_config() {
        static LoggerConfig config;
        return config;
    }

    /**
     * @brief Set a new logger backend
     * 
     * @param backend New backend function or nullptr to reset to default
     * 
     * @example
     * @code
     * // Custom backend that writes to a file
     * logger::set_backend([&file](int level, const char* cat, 
     *                            const char* file, int line, 
     *                            const std::string& msg) {
     *     file << "[" << cat << "] " << msg << std::endl;
     * });
     * @endcode
     */
    inline void set_backend(LoggerBackend backend) {
        if (backend) {
            get_config().backend = std::move(backend);
        } else {
            // Reset to default if null backend provided
            get_config().backend = internal::default_cerr_backend;
        }
    }

    /**
     * @brief Reset logger backend to default (cerr)
     */
    inline void reset_backend() {
        get_config().backend = internal::default_cerr_backend;
    }

    /**
     * @brief Set minimum log level at runtime
     * 
     * Messages below this level will be filtered out at runtime.
     * Note: Messages below LOGGER_MIN_LEVEL are still removed at compile time.
     * 
     * @param level Minimum log level (LOGGER_LEVEL_*)
     */
    inline void set_min_level(int level) {
        get_config().min_level.store(level);
    }

    /**
     * @brief Enable or disable logging at runtime
     * 
     * When disabled, all log messages are dropped regardless of level.
     * 
     * @param enabled True to enable logging, false to disable
     */
    inline void set_enabled(bool enabled) {
        get_config().enabled.store(enabled);
    }

    /**
     * @brief Check if a log level is enabled
     * 
     * @param level The log level to check
     * @return True if logging is enabled and level >= minimum level
     */
    inline bool is_level_enabled(int level) {
        auto& config = get_config();
        return config.enabled.load() &&
               level >= config.min_level.load();
    }

    /**
     * @internal
     * @brief Internal logging implementation
     */
    namespace internal {
        /**
         * @brief Core logging implementation
         * 
         * Performs runtime checks and forwards to the current backend.
         * 
         * @tparam Args Variadic template arguments for message building
         * @param level Log level
         * @param category Log category
         * @param file Source file
         * @param line Source line
         * @param args Message arguments to concatenate
         */
        template<typename... Args>
        inline void log_impl(int level, const char* category,
                             const char* file, int line, Args&&... args) {
            // Runtime check only
            if (!is_level_enabled(level)) {
                return;
            }

            // Concatenate args and call backend
            std::string message = failsafe::detail::build_message(std::forward <Args>(args)...);
            get_config().backend(level, category, file, line, message);
        }
    }

    /**
     * @brief Log with compile-time level filtering
     * 
     * This template function provides compile-time filtering based on LOGGER_MIN_LEVEL.
     * Messages below the minimum level are completely removed from the binary.
     * 
     * @tparam Level The log level (must be a compile-time constant)
     * @tparam Args Variadic template arguments for message building
     * @param category Log category string
     * @param file Source file name
     * @param line Source line number
     * @param args Message arguments to concatenate
     */
    template<int Level, typename... Args>
    inline void log_with_level(const char* category, const char* file,
                               int line, Args&&... args) {
        // Compile-time check
        if constexpr (Level >= LOGGER_MIN_LEVEL) {
            internal::log_impl(Level, category, file, line, std::forward <Args>(args)...);
        }
    }
}

/**
 * @defgroup LogMacros Logging Macros
 * @{
 */

/**
 * @brief Log a trace message with lazy evaluation
 * @param ... Variable arguments for message formatting
 * @note Arguments are only evaluated if TRACE level is enabled
 * @note Removed at compile time if LOGGER_MIN_LEVEL > TRACE
 */
#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_TRACE
#define LOG_TRACE(...) \
    (::failsafe::logger::get_config().min_level.load() > LOGGER_LEVEL_TRACE) ? void() : \
    (::failsafe::logger::log_with_level<LOGGER_LEVEL_TRACE>(LOGGER_DEFAULT_CATEGORY_STR, __FILE__, __LINE__, __VA_ARGS__), void())
#else
#define LOG_TRACE(...) ((void)0)
#endif

/**
 * @brief Log a debug message with lazy evaluation
 * @param ... Variable arguments for message formatting
 * @note Arguments are only evaluated if DEBUG level is enabled
 * @note Removed at compile time if LOGGER_MIN_LEVEL > DEBUG
 */
#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_DEBUG
#define LOG_DEBUG(...) \
    (::failsafe::logger::get_config().min_level.load() > LOGGER_LEVEL_DEBUG) ? void() : \
    (::failsafe::logger::log_with_level<LOGGER_LEVEL_DEBUG>(LOGGER_DEFAULT_CATEGORY_STR, __FILE__, __LINE__, __VA_ARGS__), void())
#else
#define LOG_DEBUG(...) ((void)0)
#endif

/**
 * @brief Log an informational message with lazy evaluation
 * @param ... Variable arguments for message formatting
 * @note Arguments are only evaluated if INFO level is enabled
 * @note Removed at compile time if LOGGER_MIN_LEVEL > INFO
 */
#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_INFO
#define LOG_INFO(...) \
    (::failsafe::logger::get_config().min_level.load() > LOGGER_LEVEL_INFO) ? void() : \
    (::failsafe::logger::log_with_level<LOGGER_LEVEL_INFO>(LOGGER_DEFAULT_CATEGORY_STR, __FILE__, __LINE__, __VA_ARGS__), void())
#else
#define LOG_INFO(...) ((void)0)
#endif

/**
 * @brief Log a warning message with lazy evaluation
 * @param ... Variable arguments for message formatting
 * @note Arguments are only evaluated if WARN level is enabled
 * @note Removed at compile time if LOGGER_MIN_LEVEL > WARN
 */
#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_WARN
#define LOG_WARN(...) \
    (::failsafe::logger::get_config().min_level.load() > LOGGER_LEVEL_WARN) ? void() : \
    (::failsafe::logger::log_with_level<LOGGER_LEVEL_WARN>(LOGGER_DEFAULT_CATEGORY_STR, __FILE__, __LINE__, __VA_ARGS__), void())
#else
#define LOG_WARN(...) ((void)0)
#endif

/**
 * @brief Log an error message with lazy evaluation
 * @param ... Variable arguments for message formatting
 * @note Arguments are only evaluated if ERROR level is enabled
 * @note Removed at compile time if LOGGER_MIN_LEVEL > ERROR
 */
#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_ERROR
#define LOG_ERROR(...) \
    (::failsafe::logger::get_config().min_level.load() > LOGGER_LEVEL_ERROR) ? void() : \
    (::failsafe::logger::log_with_level<LOGGER_LEVEL_ERROR>(LOGGER_DEFAULT_CATEGORY_STR, __FILE__, __LINE__, __VA_ARGS__), void())
#else
#define LOG_ERROR(...) ((void)0)
#endif

/**
 * @brief Log a fatal error message with lazy evaluation
 * @param ... Variable arguments for message formatting
 * @note Arguments are only evaluated if FATAL level is enabled
 * @note Removed at compile time if LOGGER_MIN_LEVEL > FATAL
 */
#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_FATAL
#define LOG_FATAL(...) \
    (::failsafe::logger::get_config().min_level.load() > LOGGER_LEVEL_FATAL) ? void() : \
    (::failsafe::logger::log_with_level<LOGGER_LEVEL_FATAL>(LOGGER_DEFAULT_CATEGORY_STR, __FILE__, __LINE__, __VA_ARGS__), void())
#else
#define LOG_FATAL(...) ((void)0)
#endif

/** @} */ // end of LogMacros group

/**
 * @defgroup CategoryLogMacros Category-based Logging Macros
 * @{
 */

/**
 * @brief Log a trace message with custom category (lazy evaluation)
 * @param category The log category string
 * @param ... Variable arguments for message formatting
 */
#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_TRACE
#define LOG_CAT_TRACE(category, ...) \
    (::failsafe::logger::get_config().min_level.load() > LOGGER_LEVEL_TRACE) ? void() : \
    (::failsafe::logger::log_with_level<LOGGER_LEVEL_TRACE>(category, __FILE__, __LINE__, __VA_ARGS__), void())
#else
#define LOG_CAT_TRACE(category, ...) ((void)0)
#endif

/**
 * @brief Log a debug message with custom category (lazy evaluation)
 * @param category The log category string
 * @param ... Variable arguments for message formatting
 */
#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_DEBUG
#define LOG_CAT_DEBUG(category, ...) \
    (::failsafe::logger::get_config().min_level.load() > LOGGER_LEVEL_DEBUG) ? void() : \
    (::failsafe::logger::log_with_level<LOGGER_LEVEL_DEBUG>(category, __FILE__, __LINE__, __VA_ARGS__), void())
#else
#define LOG_CAT_DEBUG(category, ...) ((void)0)
#endif

/**
 * @brief Log an info message with custom category (lazy evaluation)
 * @param category The log category string
 * @param ... Variable arguments for message formatting
 */
#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_INFO
#define LOG_CAT_INFO(category, ...) \
    (::failsafe::logger::get_config().min_level.load() > LOGGER_LEVEL_INFO) ? void() : \
    (::failsafe::logger::log_with_level<LOGGER_LEVEL_INFO>(category, __FILE__, __LINE__, __VA_ARGS__), void())
#else
#define LOG_CAT_INFO(category, ...) ((void)0)
#endif

/**
 * @brief Log a warning message with custom category (lazy evaluation)
 * @param category The log category string
 * @param ... Variable arguments for message formatting
 */
#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_WARN
#define LOG_CAT_WARN(category, ...) \
    (::failsafe::logger::get_config().min_level.load() > LOGGER_LEVEL_WARN) ? void() : \
    (::failsafe::logger::log_with_level<LOGGER_LEVEL_WARN>(category, __FILE__, __LINE__, __VA_ARGS__), void())
#else
#define LOG_CAT_WARN(category, ...) ((void)0)
#endif

/**
 * @brief Log an error message with custom category (lazy evaluation)
 * @param category The log category string
 * @param ... Variable arguments for message formatting
 */
#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_ERROR
#define LOG_CAT_ERROR(category, ...) \
    (::failsafe::logger::get_config().min_level.load() > LOGGER_LEVEL_ERROR) ? void() : \
    (::failsafe::logger::log_with_level<LOGGER_LEVEL_ERROR>(category, __FILE__, __LINE__, __VA_ARGS__), void())
#else
#define LOG_CAT_ERROR(category, ...) ((void)0)
#endif

/**
 * @brief Log a fatal error message with custom category (lazy evaluation)
 * @param category The log category string
 * @param ... Variable arguments for message formatting
 */
#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_FATAL
#define LOG_CAT_FATAL(category, ...) \
    (::failsafe::logger::get_config().min_level.load() > LOGGER_LEVEL_FATAL) ? void() : \
    (::failsafe::logger::log_with_level<LOGGER_LEVEL_FATAL>(category, __FILE__, __LINE__, __VA_ARGS__), void())
#else
#define LOG_CAT_FATAL(category, ...) ((void)0)
#endif

/** @} */ // end of CategoryLogMacros group

namespace failsafe::logger {
    /**
     * @brief Runtime logging function
     * 
     * Use this when the log level is not known at compile time.
     * No compile-time filtering is performed.
     * 
     * @tparam Args Variadic template arguments for message building
     * @param level Log level (determined at runtime)
     * @param category Log category
     * @param file Source file
     * @param line Source line
     * @param args Message arguments
     */
    template<typename... Args>
    void log(int level, const char* category,
             const char* file, int line, Args&&... args) {
        failsafe::logger::internal::log_impl(level, category, file, line, std::forward <Args>(args)...);
    }
}

/**
 * @defgroup ConditionalLogMacros Conditional Logging Macros
 * @{
 */

/**
 * @brief Log a message only if condition is true
 * 
 * @param condition Boolean condition to check
 * @param level Log level (runtime value)
 * @param ... Message arguments
 * 
 * @example
 * @code
 * LOG_IF(verbose_mode, LOGGER_LEVEL_DEBUG, "Detailed info:", data);
 * @endcode
 */
#define LOG_IF(condition, level, ...) \
    do { \
        if (condition) { \
            ::failsafe::logger::log(level, LOGGER_DEFAULT_CATEGORY_STR, __FILE__, __LINE__, __VA_ARGS__); \
        } \
    } while(0)

/**
 * @brief Log a message with category only if condition is true
 * 
 * @param condition Boolean condition to check
 * @param level Log level (runtime value)
 * @param category Log category string
 * @param ... Message arguments
 */
#define LOG_CAT_IF(condition, level, category, ...) \
    do { \
        if (condition) { \
            ::failsafe::logger::log(level, category, __FILE__, __LINE__, __VA_ARGS__); \
        } \
    } while(0)

/**
 * @brief Log with runtime-determined level
 * 
 * Use when log level is not a compile-time constant.
 * 
 * @param level Log level (runtime value)
 * @param ... Message arguments
 */
#define LOG_RUNTIME(level, ...) \
    ::failsafe::logger::log(level, LOGGER_DEFAULT_CATEGORY_STR, __FILE__, __LINE__, __VA_ARGS__)

/**
 * @brief Log with runtime-determined level and category
 * 
 * @param level Log level (runtime value)
 * @param category Log category string
 * @param ... Message arguments
 */
#define LOG_CAT_RUNTIME(level, category, ...) \
    ::failsafe::logger::log(level, category, __FILE__, __LINE__, __VA_ARGS__)

/** @} */ // end of ConditionalLogMacros group

