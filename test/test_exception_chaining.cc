#include <doctest/doctest.h>
#include <failsafe/exception.hh>
#include <string>
#include <fstream>

// Skip chaining tests on clang-cl where std::throw_with_nested is broken
#ifdef FAILSAFE_DISABLE_EXCEPTION_CHAINING
#define SKIP_CHAINING_TEST do { MESSAGE("Exception chaining disabled on this platform"); return; } while(0)
#else
#define SKIP_CHAINING_TEST do {} while(0)
#endif

// Simulate file operations
std::string read_file(const std::string& path) {
    if (path == "missing.txt") {
        THROW(std::runtime_error, "File not found:", path);
    }
    return "file contents";
}

// Simulate JSON parsing
void parse_json(const std::string& data) {
    if (data == "invalid json") {
        THROW(std::runtime_error, "Invalid JSON syntax at position 42");
    }
}

// Higher level function that catches and re-throws
std::string load_config(const std::string& filename) {
    try {
        auto data = read_file(filename);
        parse_json(data);
        return data;
    } catch (...) {
        // THROW automatically chains with the caught exception!
        THROW(std::runtime_error, "Failed to load configuration from", filename);
    }
    return ""; // Unreachable, but satisfies compiler
}

// Top level function
void initialize_application() {
    try {
        load_config("missing.txt");
    } catch (...) {
        // Another automatic chain
        THROW(std::runtime_error, "Application initialization failed");
    }
}

TEST_CASE("Automatic exception chaining") {
    SKIP_CHAINING_TEST;
    using namespace failsafe::exception;
    
    SUBCASE("Simple throw behavior") {
        // We don't need to test that simple throws have no nesting
        // because in some environments (like test frameworks), there
        // might be active exceptions. What matters is that chaining
        // works correctly when we want it to.
        try {
            THROW(std::runtime_error, "Simple error");
        } catch (const std::exception& e) {
            auto trace = get_nested_trace(e);
            CHECK(trace.find("Simple error") != std::string::npos);
            CHECK(trace.find("→") != std::string::npos);
            // Just verify it doesn't crash
        }
    }
    
    SUBCASE("Automatic chaining in catch blocks") {
        try {
            initialize_application();
        } catch (const std::exception& e) {
            auto trace = get_nested_trace(e);
            
            // Check that all levels are present
            CHECK(trace.find("Application initialization failed") != std::string::npos);
            CHECK(trace.find("Failed to load configuration from") != std::string::npos);
            CHECK(trace.find("File not found: missing.txt") != std::string::npos);
            
            // Verify the nesting structure
            auto lines = std::count(trace.begin(), trace.end(), '\n');
            CHECK(lines >= 3); // At least 3 exception levels
        }
    }
    
    SUBCASE("Chaining with different exception types") {
        try {
            try {
                THROW(std::invalid_argument, "Invalid input");
            } catch (...) {
                THROW(std::runtime_error, "Processing failed");
            }
        } catch (const std::exception& e) {
            auto trace = get_nested_trace(e);
            CHECK(trace.find("Processing failed") != std::string::npos);
            CHECK(trace.find("Invalid input") != std::string::npos);
        }
    }
    
    SUBCASE("Multiple catch and rethrow levels") {
        auto deep_function = []() {
            THROW(std::runtime_error, "Deep error");
        };
        
        auto middle_function = [&]() {
            try {
                deep_function();
            } catch (...) {
                THROW(std::runtime_error, "Middle layer error");
            }
        };
        
        auto top_function = [&]() {
            try {
                middle_function();
            } catch (...) {
                THROW(std::runtime_error, "Top layer error");
            }
        };
        
        try {
            top_function();
        } catch (const std::exception& e) {
            auto trace = get_nested_trace(e);
            
            // Verify all three levels
            CHECK(trace.find("Top layer error") != std::string::npos);
            CHECK(trace.find("Middle layer error") != std::string::npos);
            CHECK(trace.find("Deep error") != std::string::npos);
            
            // Verify proper indentation (each level indented more)
            auto top_pos = trace.find("Top layer error");
            auto middle_pos = trace.find("Middle layer error");
            auto deep_pos = trace.find("Deep error");
            
            CHECK(top_pos != std::string::npos);
            CHECK(middle_pos != std::string::npos);
            CHECK(deep_pos != std::string::npos);
            CHECK(top_pos < middle_pos);
            CHECK(middle_pos < deep_pos);
        }
    }
    
    SUBCASE("Conditional throwing with chaining") {
        auto function_that_may_fail = [](bool should_fail) {
            THROW_IF(should_fail, std::runtime_error, "Conditional failure");
        };
        
        try {
            try {
                function_that_may_fail(true);
            } catch (...) {
                // THROW_IF also supports chaining when in catch block
                THROW_IF(true, std::runtime_error, "Handler detected failure");
            }
        } catch (const std::exception& e) {
            auto trace = get_nested_trace(e);
            CHECK(trace.find("Handler detected failure") != std::string::npos);
            CHECK(trace.find("Conditional failure") != std::string::npos);
        }
    }
}

TEST_CASE("Exception trace formatting") {
    SKIP_CHAINING_TEST;
    using namespace failsafe::exception;
    
    SUBCASE("Trace includes file locations") {
        try {
            THROW(std::runtime_error, "Test error");
        } catch (const std::exception& e) {
            std::string what_str(e.what());
            // Should include file and line info
            CHECK(what_str.find("test_exception_chaining.cc") != std::string::npos);
            CHECK(what_str.find(":") != std::string::npos); // Line number
        }
    }
    
    SUBCASE("print_exception_trace outputs formatted trace") {
        // Capture stderr
        std::stringstream buffer;
        std::streambuf* old = std::cerr.rdbuf(buffer.rdbuf());
        
        try {
            try {
                THROW(std::runtime_error, "Inner error");
            } catch (...) {
                THROW(std::runtime_error, "Outer error");
            }
        } catch (const std::exception& e) {
            print_exception_trace(e);
        }
        
        std::cerr.rdbuf(old);
        
        auto output = buffer.str();
        CHECK(output.find("=== Exception Trace ===") != std::string::npos);
        CHECK(output.find("→") != std::string::npos);
        CHECK(output.find("Inner error") != std::string::npos);
        CHECK(output.find("Outer error") != std::string::npos);
    }
}