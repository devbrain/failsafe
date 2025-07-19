//
// Unit tests for the failsafe exception trap functionality
//

#include <doctest/doctest.h>
#include <string>
#include <sstream>
#include <cstdlib>

// Test the TRAP macros separately (they don't depend on FAILSAFE_TRAP_MODE)
#include <failsafe/exception.hh>

// Helper to capture stderr
class StderrCapture {
public:
    StderrCapture() {
        old_buf_ = std::cerr.rdbuf();
        std::cerr.rdbuf(buffer_.rdbuf());
    }
    
    ~StderrCapture() {
        std::cerr.rdbuf(old_buf_);
    }
    
    std::string get() const {
        return buffer_.str();
    }
    
    void clear() {
        buffer_.str("");
        buffer_.clear();
    }
    
private:
    std::stringstream buffer_;
    std::streambuf* old_buf_;
};

TEST_SUITE("Exception Trap Macros") {
    TEST_CASE("Basic trap macros compile correctly") {
        // These tests verify the macros compile and have correct syntax
        // We can't actually test trapping behavior in unit tests as it would
        // halt execution, but we can verify the code compiles
        
        SUBCASE("TRAP macro compiles") {
            // This would trap if executed
            // TRAP("This would trap");
            CHECK(true); // Just verify compilation
        }
        
        SUBCASE("TRAP_IF macro compiles") {
            bool should_trap = false;
            TRAP_IF(should_trap, "Would trap if true");
            CHECK(true); // We didn't trap
        }
        
        SUBCASE("TRAP_UNLESS macro compiles") {
            bool all_good = true;
            TRAP_UNLESS(all_good, "Would trap if false");
            CHECK(true); // We didn't trap
        }
    }
    
    TEST_CASE("Exception info printing") {
        StderrCapture capture;
        
        // Test the print function directly
        failsafe::exception::internal::print_exception_info("test.cc", 42, "Test message");
        
        std::string output = capture.get();
        CHECK(output.find("=== EXCEPTION TRAP ===") != std::string::npos);
        CHECK(output.find("Location: [test.cc:42]") != std::string::npos);
        CHECK(output.find("Message: Test message") != std::string::npos);
    }
    
    TEST_CASE("DEBUG_TRAP_RELEASE_THROW macro") {
        #ifdef NDEBUG
            // In release mode, it should throw
            CHECK_THROWS_AS(DEBUG_TRAP_RELEASE_THROW(std::runtime_error, "Release mode"), 
                          std::runtime_error);
        #else
            // In debug mode, it would trap, so we can't test it directly
            CHECK(true); // Just verify compilation
        #endif
    }
}

// Test throwing behavior with different trap modes
// We'll create separate test functions for each mode to avoid redefining macros

namespace trap_mode_0_tests {
    // Normal mode - should throw exceptions normally
    #undef FAILSAFE_TRAP_MODE
    #define FAILSAFE_TRAP_MODE 0
    #include <failsafe/exception.hh>
    
    TEST_SUITE("Exception Trap Mode 0 (Normal)") {
        TEST_CASE("Mode 0 throws normally") {
            CHECK_THROWS_AS(THROW(std::runtime_error, "Normal throw"), std::runtime_error);
            
            try {
                THROW(std::runtime_error, "Test message");
                FAIL("Should have thrown");
            } catch (const std::runtime_error& e) {
                std::string msg(e.what());
                CHECK(msg.find("Test message") != std::string::npos);
            }
        }
    }
}

namespace trap_mode_1_tests {
    // Mode 1 - trap then throw
    // We can't test the trap part, but we can verify it still throws after
    #undef FAILSAFE_TRAP_MODE
    #define FAILSAFE_TRAP_MODE 1
    
    // In actual use, this would trap first, but in tests we'll just verify
    // the macro compiles correctly
    TEST_SUITE("Exception Trap Mode 1 (Trap then throw)") {
        TEST_CASE("Mode 1 configuration") {
            // Just verify the mode is set correctly
            CHECK(FAILSAFE_TRAP_MODE == 1);
        }
    }
}

namespace trap_mode_2_tests {
    // Mode 2 - trap only (no throw)
    #undef FAILSAFE_TRAP_MODE
    #define FAILSAFE_TRAP_MODE 2
    
    TEST_SUITE("Exception Trap Mode 2 (Trap only)") {
        TEST_CASE("Mode 2 configuration") {
            // Just verify the mode is set correctly
            CHECK(FAILSAFE_TRAP_MODE == 2);
        }
    }
}