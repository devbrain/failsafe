/**
 * @file basic_usage.cpp
 * @brief Getting started with Failsafe library
 * 
 * This example demonstrates the basic features of the Failsafe library
 * including logging, enforcement, and exception handling.
 */

#include <failsafe/failsafe.hh>
#include <iostream>
#include <vector>
#include <cstdio>

using namespace failsafe;

// Example 1: Basic Logging
void logging_example() {
    std::cout << "\n=== Logging Example ===\n";
    
    // Simple logging at different levels
    LOG_TRACE("Application starting");
    LOG_DEBUG("Debug information: x =", 42);
    LOG_INFO("User logged in:", "john_doe");
    LOG_WARN("Memory usage at", 85, "percent");
    LOG_ERROR("Failed to connect to server:", "api.example.com");
    
    // Category-based logging
    LOG_CAT_INFO("Database", "Connected to PostgreSQL");
    LOG_CAT_DEBUG("Network", "Sending request to:", "https://api.example.com/v1/users");
    LOG_CAT_ERROR("Auth", "Invalid token provided");
    
    // Conditional logging
    bool verbose_mode = true;
    LOG_IF(verbose_mode, LOGGER_LEVEL_DEBUG, "Verbose mode is enabled");
    
    // Lazy evaluation - expensive operations only run if log level is enabled
    // This won't execute expensive_calculation if DEBUG is disabled
    LOG_DEBUG("Expensive result:", []() {
        std::cout << "  [Performing expensive calculation...]\n";
        return 42 * 1337;
    }());
}

// Example 2: Enforcement
void enforcement_example() {
    std::cout << "\n=== Enforcement Example ===\n";
    
    try {
        // Basic enforcement - throws on null
        FILE* file = fopen("test.txt", "r");
        auto safe_file = ENFORCE(file)("Failed to open test.txt");
        std::cout << "File opened successfully\n";
        fclose(safe_file);
    } catch (const std::exception& e) {
        std::cout << "Caught: " << e.what() << "\n";
    }
    
    // Enforcement with comparisons
    try {
        int port = 70000;
        ENFORCE_IN_RANGE(port, 1, 65535)("Invalid port number");
    } catch (const std::exception& e) {
        std::cout << "Caught: " << e.what() << "\n";
    }
    
    // Multiple enforcements
    auto validate_password = [](const std::string& password) {
        ENFORCE(!password.empty())("Password cannot be empty");
        ENFORCE_GE(password.length(), 8)("Password must be at least 8 characters");
        ENFORCE_LE(password.length(), 128)("Password too long");
        return password;
    };
    
    try {
        validate_password("short");
    } catch (const std::exception& e) {
        std::cout << "Password validation failed: " << e.what() << "\n";
    }
}

// Example 3: Exception Handling with Chaining
void exception_example() {
    std::cout << "\n=== Exception Handling Example ===\n";
    
    auto read_config = []() {
        THROW(std::runtime_error, "Config file not found: config.json");
    };
    
    auto load_settings = [&]() {
        try {
            read_config();
        } catch (...) {
            // Automatically chains with the caught exception
            THROW(std::runtime_error, "Failed to load application settings");
        }
    };
    
    auto initialize_app = [&]() {
        try {
            load_settings();
        } catch (...) {
            THROW(std::runtime_error, "Application initialization failed");
        }
    };
    
    try {
        initialize_app();
    } catch (const std::exception& e) {
        std::cout << "Exception trace:\n";
        std::cout << exception::get_nested_trace(e);
    }
}

// Example 4: String Formatting
void string_formatting_example() {
    std::cout << "\n=== String Formatting Example ===\n";
    
    using namespace failsafe::detail;
    
    // Automatic formatting for various types
    auto msg = build_message("Count:", 42, "Time:", std::chrono::milliseconds(1500));
    std::cout << "Built message: " << msg << "\n";
    
    // Special formatters
    std::cout << "Hex: " << build_message("Address:", hex(0xDEADBEEF)) << "\n";
    std::cout << "Binary: " << build_message("Flags:", bin(0b10101010)) << "\n";
    std::cout << "Octal: " << build_message("Permissions:", oct(0755)) << "\n";
    
    // Container formatting
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::cout << "Limited container: " 
              << build_message("First 5:", container(numbers, 5)) << "\n";
    
    // Case conversion
    std::cout << "Upper: " << build_message(uppercase("hello world")) << "\n";
    std::cout << "Lower: " << build_message(lowercase("HELLO WORLD")) << "\n";
}

// Example 5: Custom Logger Configuration
void logger_configuration_example() {
    std::cout << "\n=== Logger Configuration Example ===\n";
    
    // Save current settings
    auto original_level = logger::get_config().min_level.load();
    
    // Change minimum log level
    logger::set_min_level(LOGGER_LEVEL_INFO);
    LOG_DEBUG("This debug message won't appear");
    LOG_INFO("This info message will appear");
    
    // Create custom backend with timestamps and colors
    logger::set_backend(logger::backends::make_cerr_backend(
        true,   // show timestamp
        true,   // show thread ID
        true    // use ANSI colors
    ));
    
    LOG_INFO("Message with timestamp and colors");
    
    // Restore original settings
    logger::set_min_level(original_level);
    logger::reset_backend();
}

int main() {
    std::cout << "=== Failsafe Basic Usage Examples ===\n";
    
    try {
        logging_example();
        enforcement_example();
        exception_example();
        string_formatting_example();
        logger_configuration_example();
    } catch (const std::exception& e) {
        LOG_FATAL("Unexpected error:", e.what());
        return 1;
    }
    
    std::cout << "\n=== All examples completed successfully ===\n";
    return 0;
}