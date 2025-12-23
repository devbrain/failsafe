#include <doctest/doctest.h>

// For this test, we need all log levels to be compiled in
#define LOGGER_MIN_LEVEL LOGGER_LEVEL_TRACE

#include <failsafe/logger.hh>
#include <chrono>
#include <thread>
#include <atomic>

// Global counters to track expensive function calls
static std::atomic<int> expensive_call_count{0};
static std::atomic<int> very_expensive_call_count{0};

// Simulate expensive operations
std::string expensive_operation() {
    expensive_call_count++;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return "expensive result";
}

std::string very_expensive_operation() {
    very_expensive_call_count++;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return "very expensive result";
}

// Function that returns different types
int calculate_sum(int a, int b) {
    expensive_call_count++;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    return a + b;
}

TEST_CASE("All logging is lazy by default") {
    using namespace failsafe;
    
    // Save original log level
    auto original_level = logger::get_config().min_level.load();
    
    SUBCASE("Logging skips evaluation when disabled") {
        expensive_call_count = 0;
        
        // Set log level to ERROR (disables DEBUG)
        logger::set_min_level(LOGGER_LEVEL_ERROR);
        
        // This will NOT call expensive_operation because DEBUG is disabled
        LOG_DEBUG("Result:", expensive_operation());
        
        CHECK(expensive_call_count == 0); // Function was NOT called!
    }
    
    SUBCASE("Logging evaluates when enabled") {
        expensive_call_count = 0;
        
        // Set log level to DEBUG (enables DEBUG)
        logger::set_min_level(LOGGER_LEVEL_DEBUG);
        
        // This WILL call expensive_operation because DEBUG is enabled
        LOG_DEBUG("Result:", expensive_operation());
        
        CHECK(expensive_call_count == 1); // Function was called
    }
    
    SUBCASE("Multiple expensive operations") {
        expensive_call_count = 0;
        very_expensive_call_count = 0;
        
        // Disable debug logging
        logger::set_min_level(LOGGER_LEVEL_INFO);
        
        // None of these expensive operations should run
        LOG_DEBUG("Op1:", expensive_operation(), 
                  "Op2:", very_expensive_operation(),
                  "Sum:", calculate_sum(100, 200));
        
        CHECK(expensive_call_count == 0);
        CHECK(very_expensive_call_count == 0);
    }
    
    SUBCASE("Category-based logging is also lazy") {
        expensive_call_count = 0;
        
        logger::set_min_level(LOGGER_LEVEL_WARN);
        
        // These should not evaluate
        LOG_CAT_DEBUG("Database", "Query result:", expensive_operation());
        LOG_CAT_INFO("Network", "Stats:", very_expensive_operation());
        
        CHECK(expensive_call_count == 0);
        CHECK(very_expensive_call_count == 0);
        
        // This should evaluate (ERROR >= WARN)
        LOG_CAT_ERROR("System", "Critical:", expensive_operation());
        
        CHECK(expensive_call_count == 1);
    }
    
    SUBCASE("Performance with logging disabled") {
        const int iterations = 100;
        
        // Disable debug logging
        logger::set_min_level(LOGGER_LEVEL_ERROR);
        
        // Measure time with expensive operations in logging
        expensive_call_count = 0;
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i) {
            LOG_DEBUG("Iteration:", i, "Result:", expensive_operation());
        }
        auto elapsed = std::chrono::steady_clock::now() - start;
        
        // Should be very fast - no expensive calls made
        CHECK(expensive_call_count == 0);
        
        // Should take less than 1ms (no sleeps)
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        CHECK(ms < 10); // Very fast, no expensive operations
    }
    
    // Restore original log level
    logger::set_min_level(original_level);
}

TEST_CASE("Logging works correctly in all contexts") {
    using namespace failsafe;
    
    expensive_call_count = 0;
    logger::set_min_level(LOGGER_LEVEL_DEBUG);
    
    SUBCASE("In expression context") {
        // The log macro should work in expression contexts
        LOG_DEBUG("Value:", expensive_operation());
        int y = 10;
        CHECK(y == 10);
        CHECK(expensive_call_count == 1);
    }
    
    SUBCASE("In if statement") {
        if (true)
            LOG_DEBUG("In if:", expensive_operation());
        
        CHECK(expensive_call_count == 1);
    }
    
    SUBCASE("In loops") {
        for (int i = 0; i < 3; ++i)
            LOG_DEBUG("Loop:", i, "Op:", expensive_operation());
        
        CHECK(expensive_call_count == 3);
    }
    
    SUBCASE("All log levels are lazy") {
        // Verify that all log levels benefit from lazy evaluation
        expensive_call_count = 0;
        
        // Set level to FATAL only
        logger::set_min_level(LOGGER_LEVEL_FATAL);
        
        // None of these should evaluate
        LOG_TRACE("Trace:", expensive_operation());
        LOG_DEBUG("Debug:", expensive_operation());
        LOG_INFO("Info:", expensive_operation());
        LOG_WARN("Warn:", expensive_operation());
        LOG_ERROR("Error:", expensive_operation());
        
        CHECK(expensive_call_count == 0); // No calls made
        
        // Only FATAL should evaluate
        LOG_FATAL("Fatal:", expensive_operation());
        CHECK(expensive_call_count == 1);
    }
}