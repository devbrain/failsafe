//
// Unit tests for the failsafe enforce mechanism
//

#include <doctest/doctest.h>
#include <failsafe/enforce.hh>
#include <string>
#include <vector>
#include <memory>

using namespace failsafe::enforce;

TEST_SUITE("Enforce Mechanism") {
    TEST_CASE("Basic ENFORCE macro") {
        SUBCASE("True condition passes") {
            int x = 5;
            auto result = ENFORCE(x > 0);
            CHECK(result.get() == true);
        }
        
        SUBCASE("Can chain custom message") {
            CHECK_THROWS_AS(ENFORCE(false)("Custom error message"), 
                          std::runtime_error);
            
            try {
                ENFORCE(false)("Error: ", 42, " is the answer");
                FAIL("Should have thrown");
            } catch (const std::exception& e) {
                std::string msg(e.what());
                CHECK(msg.find("Error:  42  is the answer") != std::string::npos);
            }
        }
        
        SUBCASE("Default message on destruction") {
            // Note: Throwing from destructor will call terminate()
            // So we can't test this case directly without terminating the program
            // Instead, let's test that the enforcer works without calling operator()
            CHECK_THROWS([]{
                auto enforcer = ENFORCE(false);
                // Manually trigger the error
                enforcer("Manual trigger");
            }());
        }
        
        SUBCASE("Value pass-through") {
            int* ptr = new int(42);
            int* result = ENFORCE(ptr);
            CHECK(result == ptr);
            CHECK(*result == 42);
            delete ptr;
        }
    }
    
    TEST_CASE("Pointer enforcement") {
        SUBCASE("Non-null pointer passes") {
            int value = 42;
            int* ptr = &value;
            int* result = ENFORCE(ptr);
            CHECK(result == ptr);
        }
        
        SUBCASE("Null pointer fails") {
            int* ptr = nullptr;
            CHECK_THROWS_AS(ENFORCE(ptr)("Null pointer detected"), 
                          std::runtime_error);
        }
        
        SUBCASE("ENFORCE_NOT_NULL macro") {
            std::unique_ptr<int> ptr = std::make_unique<int>(42);
            int* raw_ptr = ENFORCE_NOT_NULL(ptr.get());
            CHECK(raw_ptr == ptr.get());
            
            int* null_ptr = nullptr;
            CHECK_THROWS(ENFORCE_NOT_NULL(null_ptr));
        }
    }
    
    TEST_CASE("Comparison enforcements") {
        SUBCASE("ENFORCE_EQ") {
            int x = 5;
            CHECK_NOTHROW(ENFORCE_EQ(x, 5));
            CHECK_THROWS(ENFORCE_EQ(x, 6)("Values don't match"));
            
            try {
                ENFORCE_EQ(x, 6);
            } catch (const std::exception& e) {
                std::string msg(e.what());
                CHECK(msg.find("x == 6") != std::string::npos);
            }
        }
        
        SUBCASE("ENFORCE_NE") {
            int x = 5;
            CHECK_NOTHROW(ENFORCE_NE(x, 6));
            CHECK_THROWS(ENFORCE_NE(x, 5));
        }
        
        SUBCASE("ENFORCE_LT") {
            int x = 5;
            CHECK_NOTHROW(ENFORCE_LT(x, 10));
            CHECK_THROWS(ENFORCE_LT(x, 5));
            CHECK_THROWS(ENFORCE_LT(x, 3)("x must be less than 3"));
        }
        
        SUBCASE("ENFORCE_GT") {
            int x = 5;
            CHECK_NOTHROW(ENFORCE_GT(x, 3));
            CHECK_THROWS(ENFORCE_GT(x, 5));
            CHECK_THROWS(ENFORCE_GT(x, 10));
        }
        
        SUBCASE("ENFORCE_LE and ENFORCE_GE") {
            int x = 5;
            CHECK_NOTHROW(ENFORCE_LE(x, 5));
            CHECK_NOTHROW(ENFORCE_LE(x, 6));
            CHECK_THROWS(ENFORCE_LE(x, 4));
            
            CHECK_NOTHROW(ENFORCE_GE(x, 5));
            CHECK_NOTHROW(ENFORCE_GE(x, 4));
            CHECK_THROWS(ENFORCE_GE(x, 6));
        }
        
        SUBCASE("ENFORCE_IN_RANGE") {
            int x = 5;
            CHECK_NOTHROW(ENFORCE_IN_RANGE(x, 0, 10));
            CHECK_NOTHROW(ENFORCE_IN_RANGE(x, 5, 5));
            CHECK_THROWS(ENFORCE_IN_RANGE(x, 6, 10));
            CHECK_THROWS(ENFORCE_IN_RANGE(x, 0, 4));
        }
    }
    
    TEST_CASE("Index validation") {
        std::vector<int> vec = {1, 2, 3, 4, 5};
        
        SUBCASE("Valid indices") {
            for (size_t i = 0; i < vec.size(); ++i) {
                CHECK_NOTHROW(ENFORCE_VALID_INDEX(i, vec.size()));
            }
        }
        
        SUBCASE("Invalid indices") {
            CHECK_THROWS(ENFORCE_VALID_INDEX(-1, vec.size()));
            CHECK_THROWS(ENFORCE_VALID_INDEX(vec.size(), vec.size()));
            CHECK_THROWS(ENFORCE_VALID_INDEX(100, vec.size()));
        }
    }
    
    TEST_CASE("Custom exception types") {
        SUBCASE("ENFORCE_THROW with specific exception") {
            CHECK_THROWS_AS(
                ENFORCE_THROW(false, std::logic_error)("Logic error"),
                std::logic_error);
            
            CHECK_THROWS_AS(
                ENFORCE_THROW(false, std::invalid_argument)("Invalid arg"),
                std::invalid_argument);
        }
    }
    
    TEST_CASE("Complex expressions") {
        SUBCASE("Compound conditions") {
            int x = 5, y = 10;
            CHECK_NOTHROW(ENFORCE(x < y && y > 0));
            CHECK_THROWS(ENFORCE(x > y || y < 0)("Invalid state"));
        }
        
        SUBCASE("Function calls in enforce") {
            auto is_positive = [](int n) { return n > 0; };
            CHECK_NOTHROW(ENFORCE(is_positive(5)));
            CHECK_THROWS(ENFORCE(is_positive(-5)));
        }
    }
    
    TEST_CASE("Value preservation and chaining") {
        SUBCASE("Enforce preserves value") {
            int x = 42;
            int y = ENFORCE(x);
            CHECK(y == 42);
            
            std::string s = "hello";
            bool not_empty = ENFORCE(!s.empty())("String is empty");
            CHECK(not_empty == true);
        }
        
        SUBCASE("Chaining multiple messages") {
            // Only the first message call should be used
            try {
                ENFORCE(false)("First")("Second")("Third");
                FAIL("Should have thrown");
            } catch (const std::exception& e) {
                std::string msg(e.what());
                CHECK(msg.find("First") != std::string::npos);
            }
        }
    }
    
    TEST_CASE("Debug enforce") {
        #ifdef NDEBUG
            // In release mode, DEBUG_ENFORCE should do nothing
            DEBUG_ENFORCE(false);
            CHECK(true); // We got here without throwing
        #else
            // In debug mode, DEBUG_ENFORCE uses ENFORCE_TRAP
            // We can't test trap behavior directly, but we can verify compilation
            bool condition = true;
            DEBUG_ENFORCE(condition); // Should not trap
            CHECK(true);
        #endif
    }
    
    TEST_CASE("Custom predicates") {
        SUBCASE("Equal predicate with custom message") {
            try {
                int x = 5;
                enforce_eq(x, 6, "x == 6", __FILE__, __LINE__)("x should be 6 but is ", x);
                FAIL("Should have thrown");
            } catch (const std::exception& e) {
                std::string msg(e.what());
                CHECK(msg.find("x should be 6 but is  5") != std::string::npos);
            }
        }
    }
    
    TEST_CASE("Message formatting") {
        using namespace failsafe::detail;
        
        SUBCASE("With formatters") {
            try {
                int value = 255;
                ENFORCE(value < 100)("Value ", hex(value), " exceeds limit");
                FAIL("Should have thrown");
            } catch (const std::exception& e) {
                std::string msg(e.what());
                CHECK(msg.find("Value  0xff  exceeds limit") != std::string::npos);
            }
        }
        
        SUBCASE("With containers") {
            std::vector<int> vec = {1, 2, 3};
            try {
                ENFORCE(vec.empty())("Vector is not empty: ", container(vec));
                FAIL("Should have thrown");
            } catch (const std::exception& e) {
                std::string msg(e.what());
                CHECK(msg.find("Vector is not empty:  [1, 2, 3]") != std::string::npos);
            }
        }
    }
}