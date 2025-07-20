#include <doctest/doctest.h>
#include <failsafe/enforce.hh>
#include <failsafe/exception.hh>

// Function that uses enforce
void* allocate_memory(size_t size) {
    // Simulate allocation failure
    return nullptr;
}

// Function that catches and re-throws
void process_data() {
    try {
        auto buffer = ENFORCE(allocate_memory(1024))("Memory allocation failed");
        // Use buffer...
    } catch (...) {
        // THROW should automatically chain with the enforcement error
        THROW(std::runtime_error, "Failed to process data");
    }
}

// Higher level function
void run_analysis() {
    try {
        process_data();
    } catch (...) {
        THROW(std::runtime_error, "Analysis failed");
    }
}

TEST_CASE("Enforce with exception chaining") {
    using namespace failsafe;
    
    SUBCASE("ENFORCE failures chain with THROW") {
        try {
            run_analysis();
        } catch (const std::exception& e) {
            auto trace = exception::get_nested_trace(e);
            
            // Should have all three levels
            CHECK(trace.find("Analysis failed") != std::string::npos);
            CHECK(trace.find("Failed to process data") != std::string::npos);
            CHECK(trace.find("Memory allocation failed") != std::string::npos);
        }
    }
    
    SUBCASE("Multiple ENFORCE in catch blocks") {
        auto validate_input = [](int value) {
            try {
                ENFORCE_GT(value, 0)("Value must be positive");
                ENFORCE_LT(value, 100)("Value must be less than 100");
            } catch (...) {
                THROW(std::invalid_argument, "Input validation failed for value:", value);
            }
        };
        
        try {
            validate_input(-5);
        } catch (const std::exception& e) {
            auto trace = exception::get_nested_trace(e);
            CHECK(trace.find("Input validation failed") != std::string::npos);
            CHECK(trace.find("Value must be positive") != std::string::npos);
        }
    }
    
    SUBCASE("Chaining with multiple enforces") {
        auto process_even_number = [](int value) {
            try {
                ENFORCE(value % 2 == 0)("Value must be even:", value);
            } catch (...) {
                THROW(std::logic_error, "Even number processing failed");
            }
        };
        
        try {
            process_even_number(7);
        } catch (const std::exception& e) {
            auto trace = exception::get_nested_trace(e);
            CHECK(trace.find("Even number processing failed") != std::string::npos);
            CHECK(trace.find("Value must be even") != std::string::npos);
        }
    }
    
    SUBCASE("Conditional ENFORCE with chaining") {
        auto conditional_check = [](bool condition, int value) {
            try {
                if (condition) {
                    ENFORCE_GT(value, 10)("Value must be greater than 10 when condition is true");
                }
            } catch (...) {
                THROW(std::runtime_error, "Conditional check failed");
            }
        };
        
        try {
            conditional_check(true, 5);
        } catch (const std::exception& e) {
            auto trace = exception::get_nested_trace(e);
            CHECK(trace.find("Conditional check failed") != std::string::npos);
            CHECK(trace.find("Value must be greater than 10") != std::string::npos);
        }
    }
}

TEST_CASE("Enforce chaining in realistic scenarios") {
    using namespace failsafe;
    
    SUBCASE("File operation with enforce") {
        auto open_config_file = [](const char* path) -> FILE* {
            return nullptr; // Simulate failure
        };
        
        auto load_configuration = [&open_config_file]() {
            try {
                auto file = ENFORCE(open_config_file("/etc/app.conf"))
                    ("Failed to open configuration file");
                // Would read file here...
                return std::string("config");
            } catch (...) {
                THROW(std::runtime_error, "Configuration loading failed");
            }
            return std::string(); // Unreachable
        };
        
        auto initialize_app = [&]() {
            try {
                auto config = load_configuration();
                // Use config...
            } catch (...) {
                THROW(std::runtime_error, "Application initialization failed");
            }
        };
        
        try {
            initialize_app();
        } catch (const std::exception& e) {
            auto trace = exception::get_nested_trace(e);
            
            // Verify complete chain
            CHECK(trace.find("Application initialization failed") != std::string::npos);
            CHECK(trace.find("Configuration loading failed") != std::string::npos);
            CHECK(trace.find("Failed to open configuration file") != std::string::npos);
            
            // All should include file location info
            CHECK(trace.find(".cc:") != std::string::npos);
        }
    }
    
    SUBCASE("Network operation with multiple enforcements") {
        struct Connection {
            int socket_fd = -1;
            bool connected = false;
        };
        
        auto connect_to_server = [](const std::string& host, int port) {
            Connection conn;
            
            try {
                // Multiple enforcements that might fail
                ENFORCE_IN_RANGE(port, 1, 65535)("Invalid port number");
                ENFORCE(!host.empty())("Host cannot be empty");
                
                // Simulate connection failure
                conn.socket_fd = -1;
                ENFORCE_NE(conn.socket_fd, -1)("Failed to create socket");
                
                return conn;
            } catch (...) {
                THROW(std::runtime_error, "Connection to", host, ":", port, "failed");
            }
            return Connection(); // Unreachable
        };
        
        try {
            connect_to_server("example.com", 8080);
        } catch (const std::exception& e) {
            auto trace = exception::get_nested_trace(e);
            CHECK(trace.find("Connection to") != std::string::npos);
            CHECK(trace.find("example.com") != std::string::npos);
            CHECK(trace.find("8080") != std::string::npos);
            CHECK(trace.find("failed") != std::string::npos);
            CHECK(trace.find("Failed to create socket") != std::string::npos);
        }
    }
}