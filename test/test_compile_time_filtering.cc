#include <doctest/doctest.h>

// Test with different compile-time log levels
#define LOGGER_MIN_LEVEL LOGGER_LEVEL_WARN
#include <failsafe/logger.hh>

#include <string>

// Function to detect if code is executed
bool g_trace_called = false;
bool g_debug_called = false;
bool g_info_called = false;
bool g_warn_called = false;
bool g_error_called = false;
bool g_fatal_called = false;

std::string trace_func() { g_trace_called = true; return "trace"; }
std::string debug_func() { g_debug_called = true; return "debug"; }
std::string info_func() { g_info_called = true; return "info"; }
std::string warn_func() { g_warn_called = true; return "warn"; }
std::string error_func() { g_error_called = true; return "error"; }
std::string fatal_func() { g_fatal_called = true; return "fatal"; }

TEST_CASE("Compile-time filtering removes code below LOGGER_MIN_LEVEL") {
    using namespace failsafe;
    
    // Save original runtime level
    auto original_level = logger::get_config().min_level.load();
    
    // Set runtime level to allow all messages
    logger::set_min_level(LOGGER_LEVEL_TRACE);
    
    SUBCASE("Macros below MIN_LEVEL expand to no-op") {
        // Reset flags
        g_trace_called = false;
        g_debug_called = false;
        g_info_called = false;
        g_warn_called = false;
        g_error_called = false;
        g_fatal_called = false;
        
        // These should be compiled out (MIN_LEVEL is WARN)
        LOG_TRACE("Message:", trace_func());
        LOG_DEBUG("Message:", debug_func());
        LOG_INFO("Message:", info_func());
        
        // These should execute
        LOG_WARN("Message:", warn_func());
        LOG_ERROR("Message:", error_func());
        LOG_FATAL("Message:", fatal_func());
        
        // Verify compile-time filtering
        CHECK(g_trace_called == false);  // Compiled out
        CHECK(g_debug_called == false);  // Compiled out
        CHECK(g_info_called == false);   // Compiled out
        CHECK(g_warn_called == true);    // Executed
        CHECK(g_error_called == true);   // Executed
        CHECK(g_fatal_called == true);   // Executed
    }
    
    SUBCASE("Category macros below MIN_LEVEL expand to no-op") {
        // Reset flags
        g_trace_called = false;
        g_debug_called = false;
        g_info_called = false;
        g_warn_called = false;
        g_error_called = false;
        g_fatal_called = false;
        
        // These should be compiled out (MIN_LEVEL is WARN)
        LOG_CAT_TRACE("Cat", "Message:", trace_func());
        LOG_CAT_DEBUG("Cat", "Message:", debug_func());
        LOG_CAT_INFO("Cat", "Message:", info_func());
        
        // These should execute
        LOG_CAT_WARN("Cat", "Message:", warn_func());
        LOG_CAT_ERROR("Cat", "Message:", error_func());
        LOG_CAT_FATAL("Cat", "Message:", fatal_func());
        
        // Verify compile-time filtering
        CHECK(g_trace_called == false);  // Compiled out
        CHECK(g_debug_called == false);  // Compiled out
        CHECK(g_info_called == false);   // Compiled out
        CHECK(g_warn_called == true);    // Executed
        CHECK(g_error_called == true);   // Executed
        CHECK(g_fatal_called == true);   // Executed
    }
    
    SUBCASE("Conditional macros always evaluate (runtime decision)") {
        // Reset flags
        g_debug_called = false;
        
        // LOG_IF uses runtime level checking, not compile-time
        LOG_IF(true, LOGGER_LEVEL_DEBUG, "Message:", debug_func());
        
        // This will execute because LOG_IF doesn't use compile-time filtering
        // but the message won't be logged due to runtime level
        CHECK(g_debug_called == true);
    }
    
    // Restore original level
    logger::set_min_level(original_level);
}

// Test that macros expand correctly
TEST_CASE("Compile-time filtered macros expand to void expression") {
    // These should compile without warnings even in expression context
    int x = 5;
    
    // Should compile as ((void)0) which is a valid expression
    (LOG_TRACE("test"), x = 10);
    CHECK(x == 10);
    
    // Should work in conditional expressions
    true ? LOG_DEBUG("test") : LOG_INFO("test");
    
    // Should work in comma expressions
    int y = (LOG_INFO("test"), 42);
    CHECK(y == 42);
}