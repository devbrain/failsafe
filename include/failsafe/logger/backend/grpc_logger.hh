/**
 * @file grpc_logger.hh
 * @brief Integration backend for gRPC/Abseil logging
 * 
 * @details
 * This backend integrates the failsafe logger with gRPC's Abseil-based logging
 * system. It provides:
 * - Custom LogSink that forwards Abseil logs to failsafe logger
 * - Automatic severity level mapping between systems
 * - Synchronization of log verbosity settings
 * - Proper initialization and shutdown functions
 * 
 * The integration ensures that all gRPC internal logs are captured and
 * formatted consistently with your application logs.
 * 
 * @example
 * @code
 * // Initialize at application startup
 * logger::grpc::init_grpc_logging();
 * 
 * // Sync verbosity when logger level changes
 * logger::grpc::sync_grpc_verbosity();
 * 
 * // Shutdown at application exit
 * logger::grpc::shutdown_grpc_logging();
 * @endcode
 */
#pragma once

// ---------------- gRPC / Abseil logging -------------------------------------
#include <absl/log/log.h>
#include <absl/log/log_sink.h>
#include <absl/log/log_sink_registry.h>   // AddLogSink / RemoveLogSink
#include <absl/log/globals.h>             // SetMinLogLevel
#include <absl/log/initialize.h>
//#include <absl/log/verbosity.h>           // SetVLogLevel

// ---------------- your logger ------------------------------------------------
#include <failsafe/logger.hh>               // ::logger::log(), level constants
#include <string>

/**
 * @namespace failsafe::logger::grpc
 * @brief gRPC/Abseil logging integration
 */
namespace failsafe::logger::grpc {
    /**
     * @brief Map Abseil log severity to failsafe logger levels
     * 
     * Converts between Abseil's LogSeverity enum and failsafe's
     * integer log levels.
     * 
     * @param s Abseil LogSeverity value
     * @return Corresponding LOGGER_LEVEL_* constant
     */
    inline int map_absl_severity(absl::LogSeverity s) {
        switch (s) {
            case absl::LogSeverity::kInfo: return LOGGER_LEVEL_INFO;
            case absl::LogSeverity::kWarning: return LOGGER_LEVEL_WARN;
            case absl::LogSeverity::kError: return LOGGER_LEVEL_ERROR;
            case absl::LogSeverity::kFatal: return LOGGER_LEVEL_ERROR;
            default: return LOGGER_LEVEL_INFO;
        }
    }

    /**
     * @brief Custom Abseil LogSink that forwards to failsafe logger
     * 
     * @details
     * This sink receives all log messages from the Abseil logging system
     * (used by gRPC) and forwards them to the failsafe logger with proper
     * severity mapping and category labeling.
     * 
     * All gRPC logs will appear with category "gRPC" in the failsafe logger.
     */
    class GrpcLogSink : public absl::LogSink {
        public:
            /**
             * @brief Process log entry from Abseil
             * 
             * Extracts relevant information from the Abseil LogEntry and
             * forwards it to the failsafe logger.
             * 
             * @param e Abseil log entry containing message and metadata
             */
            void Send(const absl::LogEntry& e) override {
                auto level = map_absl_severity(e.log_severity());
                std::string text{e.text_message()};
                ::logger::log(
                    level,
                    "gRPC",
                    e.source_filename().data(), // const char*
                    e.source_line(),
                    text
                );
            }
    };

    /**
     * @brief Get singleton instance of GrpcLogSink
     * 
     * @return Reference to the global GrpcLogSink instance
     * @internal
     */
    inline GrpcLogSink& sink_instance() {
        static GrpcLogSink s;
        return s;
    }

    /**
     * @defgroup GrpcIntegration gRPC Integration Functions
     * @{
     */

    /**
     * @brief Initialize gRPC logging integration
     * 
     * @details
     * Call this once at application startup to:
     * - Initialize Abseil logging system
     * - Set stderr threshold to FATAL (suppresses direct stderr output)
     * - Register the custom LogSink to forward logs to failsafe logger
     * 
     * Safe to call multiple times - registration only happens once.
     * 
     * @example
     * @code
     * int main() {
     *     // Initialize failsafe logger first
     *     logger::set_backend(make_cerr_backend());
     *     
     *     // Then initialize gRPC integration
     *     logger::grpc::init_grpc_logging();
     *     
     *     // Your gRPC code here...
     * }
     * @endcode
     */
    inline void init_grpc_logging() {
        absl::InitializeLog();
        absl::SetStderrThreshold(absl::LogSeverity::kFatal);
        static bool registered = false;
        if (!registered) {
            absl::AddLogSink(&sink_instance());
            registered = true;
        }
    }

    /**
     * @brief Shutdown gRPC logging integration
     * 
     * Removes the custom LogSink from Abseil's registry.
     * Call this at application shutdown for clean teardown.
     */
    inline void shutdown_grpc_logging() {
        absl::RemoveLogSink(&sink_instance());
    }

    /**
     * @brief Synchronize gRPC verbosity with failsafe logger level
     * 
     * @details
     * Updates Abseil's minimum log level and VLOG settings to match
     * the current failsafe logger configuration. This ensures that:
     * - When failsafe logger is set to INFO or below, Abseil logs INFO+
     * - When failsafe logger is above INFO, Abseil only logs ERROR+
     * - VLOG is enabled when failsafe logger is at DEBUG or TRACE
     * 
     * Call this whenever you change the failsafe logger's minimum level
     * to keep both systems in sync.
     * 
     * @example
     * @code
     * // Change logger level
     * logger::set_min_level(LOGGER_LEVEL_DEBUG);
     * 
     * // Sync gRPC to match
     * logger::grpc::sync_grpc_verbosity();
     * @endcode
     */
    inline void sync_grpc_verbosity() {
        const int min_lvl = ::logger::get_config().min_level.load();

        // Minimum severity for Abseil
        absl::LogSeverity abs_min =
            (min_lvl <= LOGGER_LEVEL_INFO)
                ? absl::LogSeverity::kInfo
                : absl::LogSeverity::kError;

        absl::SetMinLogLevel(absl::LogSeverityAtLeast(abs_min));

        // VLOG control (optional)
        absl::SetVLogLevel("*", (min_lvl <= LOGGER_LEVEL_DEBUG) ? 1 : 0);
    }

    /** @} */ // end of GrpcIntegration group
} // namespace failsafe::logger::grpc
