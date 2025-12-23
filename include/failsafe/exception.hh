/**
 * @file exception.hh
 * @brief Exception throwing macros with message formatting and optional debug trapping
 * 
 * @details
 * This header provides a flexible exception system with the following features:
 * - Variadic message formatting similar to the logger
 * - Automatic source location information
 * - Configurable default exception type
 * - Optional debug trap modes for debugging
 * - Support for any exception type with string constructor
 * 
 * @example
 * @code
 * // Basic exception throwing
 * THROW(std::runtime_error, "Operation failed:", error_code);
 * 
 * // Using default exception type
 * THROW_DEFAULT("Invalid value:", value, "expected:", expected);
 * 
 * // Conditional throwing
 * THROW_IF(ptr == nullptr, std::invalid_argument, "Null pointer");
 * THROW_UNLESS(file.is_open(), std::runtime_error, "Failed to open:", filename);
 * 
 * // Debug trap modes (set FAILSAFE_TRAP_MODE):
 * // 0: Normal exception throwing (default)
 * // 1: Trap to debugger then throw
 * // 2: Trap to debugger only (no throw)
 * @endcode
 */
#pragma once

#include <exception>
#include <stdexcept>
#include <string>
#include <sstream>
#include <utility>
#include <iostream>

#include <failsafe/detail/string_utils.hh>
#include <failsafe/detail/psnip_debug_trap.h>
#include <failsafe/detail/location_format.hh>

/**
 * @brief Default exception type for THROW_DEFAULT macro
 * 
 * Can be overridden by defining before including this header.
 * Default is std::runtime_error.
 */
#ifndef FAILSAFE_DEFAULT_EXCEPTION
#define FAILSAFE_DEFAULT_EXCEPTION std::runtime_error
#endif

/**
 * @defgroup TrapModes Debug Trap Modes
 * @{
 */

/**
 * @brief Debug trap mode configuration
 * 
 * Controls behavior when exceptions are thrown:
 * - 0: Normal exception throwing (default)
 * - 1: Trap to debugger and then throw
 * - 2: Trap to debugger only (no throw)
 * 
 * Can be set by defining FAILSAFE_TRAP_MODE before including this header.
 * If FAILSAFE_TRAP_ON_THROW is defined, defaults to mode 2.
 */
#ifndef FAILSAFE_TRAP_MODE
#ifdef FAILSAFE_TRAP_ON_THROW
#define FAILSAFE_TRAP_MODE 2
#else
#define FAILSAFE_TRAP_MODE 0
#endif
#endif

/** @} */ // end of TrapModes group

/**
 * @namespace failsafe::exception
 * @brief Exception handling utilities
 */
namespace failsafe::exception {
    /**
     * @namespace failsafe::exception::internal
     * @brief Internal implementation details (not part of public API)
     */
    namespace internal {
        /**
         * @brief Helper to create exception with formatted message
         * @internal
         */
        template<typename Exception, typename... Args>
        [[noreturn]] inline void throw_exception_with_message(const char* file, int line, Args&&... args) {
            std::ostringstream oss;

            // Add formatted location
            failsafe::detail::append_location(oss, file, line);
            oss << " ";

            // Build the message using the same formatting as logger
            std::string message = failsafe::detail::build_message(std::forward <Args>(args)...);
            oss << message;

            throw Exception(oss.str());
        }

        /**
         * @brief Type trait to check if exception has string constructor
         * @internal
         */
        template<typename Exception>
        struct has_string_constructor {
            template<typename T>
            static auto test(int) -> decltype(T(std::string{}), std::true_type{});

            template<typename>
            static std::false_type test(...);

            static constexpr bool value = decltype(test <Exception>(0))::value;
        };

        /**
         * @brief Print exception information before debug trap
         * 
         * @param file Source file name
         * @param line Source line number
         * @param message Exception message
         * @internal
         */
        inline void print_exception_info(const char* file, int line, const std::string& message) {
            std::cerr << "\n=== EXCEPTION TRAP ===\n"
                << "Location: " << failsafe::detail::format_location(file, line) << "\n"
                << "Message: " << message << "\n"
                << "======================\n" << std::flush;
        }

        /**
         * @brief Main exception throwing implementation
         * 
         * Handles different trap modes and exception types.
         * Automatically chains with current exception if one exists.
         * 
         * @tparam Exception The exception type to throw
         * @tparam Args Variadic arguments for message formatting
         * @param file Source file name
         * @param line Source line number
         * @param args Message arguments
         * @internal
         */
        template<typename Exception, typename... Args>
        [[noreturn]]
        inline void throw_exception(const char* file, int line, Args&&... args) {
            // Build the message first
            std::string message = failsafe::detail::build_message(std::forward <Args>(args)...);

            // Handle debug trap modes
#if FAILSAFE_TRAP_MODE == 1
                // Mode 1: Trap then throw
                print_exception_info(file, line, message);
                psnip_trap();
                // Continue to throw after trap
#elif FAILSAFE_TRAP_MODE == 2
                // Mode 2: Trap only (no throw)
                print_exception_info(file, line, message);
                psnip_trap();
                // In release builds without debugger, we still need to terminate
                std::terminate();
#endif

            // Normal throw (mode 0) or after trap (mode 1)
#if FAILSAFE_TRAP_MODE != 2
            if constexpr (has_string_constructor <Exception>::value) {
                std::ostringstream oss;
                failsafe::detail::append_location(oss, file, line);
                oss << " " << message;
                
                // Check if there's a current exception to chain with
                if (std::current_exception()) {
                    // Automatically chain with the current exception
                    std::throw_with_nested(Exception(oss.str()));
                } else {
                    // No current exception, throw normally
                    throw Exception(oss.str());
                }
            } else {
                // For exceptions without string constructor
                if (std::current_exception()) {
                    std::throw_with_nested(Exception());
                } else {
                    throw Exception();
                }
            }
#endif
        }
    } // namespace failsafe::exception::internal
    
    /**
     * @brief Extract full exception trace from nested exceptions
     * 
     * Recursively extracts the exception chain created by automatic chaining
     * in the THROW macro when used within catch blocks.
     * 
     * @param e The exception to extract trace from
     * @param indent_level Current indentation level (for nested exceptions)
     * @return String containing the full exception trace
     * 
     * @example
     * @code
     * try {
     *     initialize_app();
     * } catch (const std::exception& e) {
     *     std::cerr << failsafe::exception::get_nested_trace(e) << std::endl;
     *     // Output:
     *     // → Failed to initialize application [at main.cc:45]
     *     //   → Failed to load config [at config.cc:23]
     *     //     → File not found: app.json [at file_io.cc:89]
     * }
     * @endcode
     */
    inline std::string get_nested_trace(const std::exception& e, unsigned int indent_level = 0) {
        std::string indent(indent_level * 2u, ' ');
        std::string result = indent + "→ " + e.what() + "\n";
        
        try {
            std::rethrow_if_nested(e);
        } catch (const std::exception& nested) {
            result += get_nested_trace(nested, indent_level + 1);
        } catch (...) {
            result += indent + "  → [unknown nested exception]\n";
        }
        
        return result;
    }
    
    /**
     * @brief Print exception trace to stderr
     * 
     * Convenience function that prints the full nested exception trace
     * to standard error with a header.
     * 
     * @param e The exception to print
     */
    inline void print_exception_trace(const std::exception& e) {
        std::cerr << "\n=== Exception Trace ===\n" 
                  << get_nested_trace(e)
                  << "=====================\n" << std::flush;
    }
    
} // namespace failsafe::exception

/**
 * @defgroup ExceptionMacros Exception Throwing Macros
 * @{
 */

/**
 * @brief Throw an exception with specified type and formatted message
 * 
 * @param ExceptionType The exception class to throw
 * @param ... Variable arguments for message formatting
 * 
 * @example
 * @code
 * THROW(std::runtime_error, "Operation failed with code:", error_code);
 * @endcode
 */
#define THROW(ExceptionType, ...) \
    ::failsafe::exception::internal::throw_exception< ExceptionType >( \
        __FILE__, __LINE__, __VA_ARGS__)

/**
 * @brief Throw the default exception type with formatted message
 * 
 * Uses FAILSAFE_DEFAULT_EXCEPTION (default: std::runtime_error)
 * 
 * @param ... Variable arguments for message formatting
 */
#define THROW_DEFAULT(...) \
    THROW(FAILSAFE_DEFAULT_EXCEPTION, __VA_ARGS__)

/**
 * @brief Conditionally throw an exception
 * 
 * @param condition Boolean condition to check
 * @param ExceptionType The exception class to throw
 * @param ... Variable arguments for message formatting
 */
#define THROW_IF(condition, ExceptionType, ...) \
    do { \
        if (condition) { \
            THROW(ExceptionType, __VA_ARGS__); \
        } \
    } while(0)

/**
 * @brief Conditionally throw the default exception type
 * 
 * @param condition Boolean condition to check
 * @param ... Variable arguments for message formatting
 */
#define THROW_DEFAULT_IF(condition, ...) \
    THROW_IF(condition, FAILSAFE_DEFAULT_EXCEPTION, __VA_ARGS__)

/**
 * @brief Throw exception unless condition is true (assert-like)
 * 
 * @param condition Boolean condition that must be true
 * @param ExceptionType The exception class to throw if false
 * @param ... Variable arguments for message formatting
 */
#define THROW_UNLESS(condition, ExceptionType, ...) \
    THROW_IF(!(condition), ExceptionType, __VA_ARGS__)

/**
 * @brief Throw default exception unless condition is true
 * 
 * @param condition Boolean condition that must be true
 * @param ... Variable arguments for message formatting
 */
#define THROW_DEFAULT_UNLESS(condition, ...) \
    THROW_UNLESS(condition, FAILSAFE_DEFAULT_EXCEPTION, __VA_ARGS__)

/** @} */ // end of ExceptionMacros group

/**
 * @defgroup ConvenienceMacros Convenience Exception Macros
 * @{
 */

/** @brief Throw std::runtime_error */
#define THROW_RUNTIME(...) THROW(std::runtime_error, __VA_ARGS__)

/** @brief Throw std::logic_error */
#define THROW_LOGIC(...) THROW(std::logic_error, __VA_ARGS__)

/** @brief Throw std::invalid_argument */
#define THROW_INVALID_ARG(...) THROW(std::invalid_argument, __VA_ARGS__)

/** @brief Throw std::out_of_range */
#define THROW_OUT_OF_RANGE(...) THROW(std::out_of_range, __VA_ARGS__)

/** @brief Throw std::length_error */
#define THROW_LENGTH(...) THROW(std::length_error, __VA_ARGS__)

/** @brief Throw std::domain_error */
#define THROW_DOMAIN(...) THROW(std::domain_error, __VA_ARGS__)

/** @} */ // end of ConvenienceMacros group

/**
 * @defgroup TrapMacros Debug Trap Macros
 * @brief Direct control over debug trap behavior
 * @{
 */

/**
 * @brief Always trap to debugger (no throw)
 * 
 * Prints exception info and traps to debugger.
 * Never returns - calls std::terminate() after trap.
 * 
 * @param ... Variable arguments for message formatting
 */
#define TRAP(...) \
    do { \
        ::failsafe::exception::internal::print_exception_info(__FILE__, __LINE__, \
            ::failsafe::detail::build_message(__VA_ARGS__)); \
        psnip_trap(); \
        std::terminate(); \
    } while(0)

/**
 * @brief Conditionally trap to debugger
 * 
 * @param condition Boolean condition to check
 * @param ... Variable arguments for message formatting
 */
#define TRAP_IF(condition, ...) \
    do { \
        if (condition) { \
            TRAP(__VA_ARGS__); \
        } \
    } while(0)

/**
 * @brief Trap unless condition is true (assert-like)
 * 
 * @param condition Boolean condition that must be true
 * @param ... Variable arguments for message formatting
 */
#define TRAP_UNLESS(condition, ...) \
    TRAP_IF(!(condition), __VA_ARGS__)

/**
 * @brief Trap in debug builds, throw in release builds
 * 
 * Provides different behavior based on NDEBUG:
 * - Debug builds: Trap to debugger
 * - Release builds: Throw exception
 * 
 * @param ExceptionType The exception class to throw in release
 * @param ... Variable arguments for message formatting
 */
#ifdef NDEBUG
    #define DEBUG_TRAP_RELEASE_THROW(ExceptionType, ...) \
        THROW(ExceptionType, __VA_ARGS__)
#else
#define DEBUG_TRAP_RELEASE_THROW(ExceptionType, ...) \
        TRAP(__VA_ARGS__)
#endif

/** @} */ // end of TrapMacros group
