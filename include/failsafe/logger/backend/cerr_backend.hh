/**
 * @file cerr_backend.hh
 * @brief Thread-safe stderr backend for the failsafe logger
 * 
 * @details
 * Provides a configurable backend that outputs log messages to std::cerr.
 * Features include:
 * - Thread-safe output with mutex protection
 * - Optional timestamp with millisecond precision
 * - Optional thread ID display
 * - Optional ANSI color codes for different log levels
 * - Customizable formatting
 */
#pragma once

#include <failsafe/logger.hh>
#include <failsafe/detail/location_format.hh>

#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <thread>
#include <memory>
#include <sstream>

#if defined(_MSC_VER)
#if !defined(NOMINMAX)
#define NOMINMAX
#define FAILSAFE_UNDEF_NOMINMAX
#endif
#endif

#include <termcolor/termcolor.hpp>

/**
 * @namespace failsafe::logger::backends
 * @brief Logger backend implementations
 */
namespace failsafe::logger::backends {
    /**
     * @brief Thread-safe stderr backend with configurable output features
     *
     * @details
     * CerrBackend provides a thread-safe logging backend that outputs to std::cerr.
     * It supports various formatting options including timestamps, thread IDs, and
     * ANSI color codes for different log levels.
     *
     * Features:
     * - Thread-safe output using mutex protection
     * - Optional millisecond-precision timestamps
     * - Optional thread ID display
     * - Optional ANSI color codes for log levels
     * - Automatic formatting with location information
     *
     * @example
     * @code
     * // Create a backend with all features enabled
     * auto backend = CerrBackend(true, true, true);
     *
     * // Use as logger backend
     * logger::set_backend(backend);
     *
     * // Or use the factory function
     * logger::set_backend(make_cerr_backend(true, false, true));
     * @endcode
     */
    class CerrBackend {
        private:
            mutable std::mutex mutex_; ///< Mutex for thread-safe output
            bool show_timestamp_; ///< Whether to show timestamps
            bool show_thread_id_; ///< Whether to show thread IDs
            bool use_colors_; ///< Whether to use ANSI color codes

        public:
            /**
             * @brief Construct a CerrBackend with specified features
             *
             * @param show_timestamp Enable timestamp display (default: true)
             * @param show_thread_id Enable thread ID display (default: false)
             * @param use_colors Enable ANSI color codes (default: true)
             */
            CerrBackend(bool show_timestamp = true,
                        bool show_thread_id = false,
                        bool use_colors = true)
                : show_timestamp_(show_timestamp)
                  , show_thread_id_(show_thread_id)
                  , use_colors_(use_colors) {
            }

            /**
             * @brief Log a message to stderr
             *
             * Thread-safe logging function that formats and outputs the log message
             * to std::cerr with configured formatting options.
             *
             * @param level Log level (LOGGER_LEVEL_* constants)
             * @param category Log category string
             * @param file Source file name
             * @param line Source line number
             * @param message The formatted log message
             */
            void operator()(int level, const char* category,
                            const char* file, int line,
                            const std::string& message) {
                std::lock_guard <std::mutex> lock(mutex_);

                // Add timestamp with millisecond precision
                if (show_timestamp_) {
                    auto now = std::chrono::system_clock::now();
                    auto time_t = std::chrono::system_clock::to_time_t(now);
                    auto ms = std::chrono::duration_cast <std::chrono::milliseconds>(
                                  now.time_since_epoch()) % 1000;

                    std::cerr << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
                    std::cerr << '.' << std::setfill('0') << std::setw(3) << ms.count() << " ";
                }

                // Add thread ID in brackets
                if (show_thread_id_) {
                    std::cerr << "[" << std::this_thread::get_id() << "] ";
                }

                // Apply ANSI color codes based on log level
                if (use_colors_) {
                    switch (level) {
                        case LOGGER_LEVEL_TRACE: std::cerr << termcolor::white;
                            break; // White
                        case LOGGER_LEVEL_DEBUG: std::cerr << termcolor::cyan;
                            break; // Cyan
                        case LOGGER_LEVEL_INFO: std::cerr << termcolor::green;
                            break; // Green
                        case LOGGER_LEVEL_WARN: std::cerr << termcolor::yellow;
                            break; // Yellow
                        case LOGGER_LEVEL_ERROR: std::cerr << termcolor::red;
                            break; // Red
                        default: std::cerr << termcolor::magenta;
                            break; // Magenta
                    }
                }

                // Log level and category
                std::cerr << "[" << logger::internal::level_to_string(level) << "] "
                    << "[" << category << "] ";

                // Reset color
                if (use_colors_) {
                    std::cerr << termcolor::reset;
                }

                // Location and message
                std::cerr << ::failsafe::detail::format_location(file, line)
                    << " - " << message << std::endl;
            }
    };

    /**
     * @brief Factory function to create a CerrBackend
     *
     * Creates a CerrBackend wrapped in a LoggerBackend for use with
     * the logger system.
     *
     * @param show_timestamp Enable timestamp display (default: true)
     * @param show_thread_id Enable thread ID display (default: false)
     * @param use_colors Enable ANSI color codes (default: true)
     * @return LoggerBackend containing the configured CerrBackend
     *
     * @example
     * @code
     * // Set up logger with custom cerr backend
     * logger::set_backend(make_cerr_backend(true, true, false));
     * @endcode
     */
    inline logger::LoggerBackend make_cerr_backend(bool show_timestamp = true,
                                                   bool show_thread_id = false,
                                                   bool use_colors = true) {
        // Use shared_ptr to handle the non-copyable mutex in CerrBackend
        auto backend = std::make_shared <CerrBackend>(show_timestamp, show_thread_id, use_colors);
        return [backend](int level, const char* category, const char* file, int line,
                         const std::string& message) {
            (*backend)(level, category, file, line, message);
        };
    }

    /**
     * @brief Simple stateless cerr backend function
     *
     * Provides a basic backend implementation without any state or
     * configuration options. Outputs plain text without colors or timestamps.
     * Thread-safety depends on std::cerr implementation.
     *
     * @param level Log level (LOGGER_LEVEL_* constants)
     * @param category Log category string
     * @param file Source file name
     * @param line Source line number
     * @param message The formatted log message
     *
     * @example
     * @code
     * // Use as a simple backend
     * logger::set_backend(simple_cerr_backend);
     * @endcode
     */
    inline void simple_cerr_backend(int level, const char* category,
                                    const char* file, int line,
                                    const std::string& message) {
        std::cerr << "[" << logger::internal::level_to_string(level) << "] "
            << "[" << category << "] "
            << ::failsafe::detail::format_location(file, line) << " - "
            << message << std::endl;
    }
}

#if defined(FAILSAFE_UNDEF_NOMINMAX)
#if defined(NOMINMAX)
#undef NOMINMAX
#endif
#endif