//
// Unit tests for the failsafe exception macros
//

#include <doctest/doctest.h>
#include <failsafe/exception.hh>
#include <string>
#include <stdexcept>
#include <exception>

// Custom exception for testing
class CustomException : public std::exception {
public:
    explicit CustomException(const std::string& msg) : message_(msg) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

// Exception without string constructor
class NoStringException : public std::exception {
public:
    const char* what() const noexcept override { return "NoStringException"; }
};

// Test with custom default exception type
namespace custom_default_tests {
    // Redefine default exception for these tests
    #undef FAILSAFE_DEFAULT_EXCEPTION
    #define FAILSAFE_DEFAULT_EXCEPTION std::logic_error

TEST_SUITE("Exception Macros") {
    TEST_CASE("THROW macro with custom exception type") {
        SUBCASE("Basic throw with message") {
            CHECK_THROWS_AS(THROW(std::runtime_error, "Test error"), std::runtime_error);
            
            try {
                THROW(std::runtime_error, "Test error");
                FAIL("Exception should have been thrown");
            } catch (const std::runtime_error& e) {
                std::string msg(e.what());
                CHECK(msg.find("Test error") != std::string::npos);
                CHECK(msg.find("test_exception.cc:") != std::string::npos);
                CHECK(msg.find("] Test error") != std::string::npos);
            }
        }

        SUBCASE("Throw with formatted message") {
            try {
                int value = 42;
                THROW(std::invalid_argument, "Invalid value: ", value, ", expected: ", 0);
                FAIL("Exception should have been thrown");
            } catch (const std::invalid_argument& e) {
                std::string msg(e.what());
                CHECK(msg.find("Invalid value:  42 , expected:  0") != std::string::npos);
            }
        }

        SUBCASE("Custom exception with string constructor") {
            CHECK_THROWS_AS(THROW(CustomException, "Custom error"), CustomException);
            
            try {
                THROW(CustomException, "Error code: ", 404);
                FAIL("Exception should have been thrown");
            } catch (const CustomException& e) {
                std::string msg(e.what());
                CHECK(msg.find("Error code:  404") != std::string::npos);
            }
        }

        SUBCASE("Exception without string constructor") {
            CHECK_THROWS_AS(THROW(NoStringException, "This message will be ignored"), NoStringException);
            
            try {
                THROW(NoStringException, "Ignored");
                FAIL("Exception should have been thrown");
            } catch (const NoStringException& e) {
                CHECK(std::string(e.what()) == "NoStringException");
            }
        }
    }

    TEST_CASE("THROW_DEFAULT macro") {
        SUBCASE("Uses configured default exception") {
            // Default is now std::logic_error
            CHECK_THROWS_AS(THROW_DEFAULT("Default error"), std::logic_error);
            
            try {
                THROW_DEFAULT("Error with value: ", 123);
                FAIL("Exception should have been thrown");
            } catch (const std::logic_error& e) {
                std::string msg(e.what());
                CHECK(msg.find("Error with value:  123") != std::string::npos);
            }
        }
    }

    TEST_CASE("Conditional throw macros") {
        SUBCASE("THROW_IF with true condition") {
            bool condition = true;
            CHECK_THROWS_AS(THROW_IF(condition, std::runtime_error, "Condition was true"), 
                          std::runtime_error);
        }

        SUBCASE("THROW_IF with false condition") {
            bool condition = false;
            CHECK_NOTHROW(THROW_IF(condition, std::runtime_error, "Should not throw"));
        }

        SUBCASE("THROW_DEFAULT_IF") {
            bool true_cond = true;
            bool false_cond = false;
            CHECK_THROWS_AS(THROW_DEFAULT_IF(true_cond, "Math still works"), std::logic_error);
            CHECK_NOTHROW(THROW_DEFAULT_IF(false_cond, "Should not throw"));
        }

        SUBCASE("THROW_UNLESS with true condition") {
            bool valid = true;
            CHECK_NOTHROW(THROW_UNLESS(valid, std::runtime_error, "Should not throw"));
        }

        SUBCASE("THROW_UNLESS with false condition") {
            bool valid = false;
            CHECK_THROWS_AS(THROW_UNLESS(valid, std::runtime_error, "Validation failed"), 
                          std::runtime_error);
        }

        SUBCASE("THROW_DEFAULT_UNLESS") {
            int value = 5;
            CHECK_NOTHROW(THROW_DEFAULT_UNLESS(value > 0, "Value must be positive"));
            CHECK_THROWS_AS(THROW_DEFAULT_UNLESS(value < 0, "Value is not negative"), 
                          std::logic_error);
        }
    }

    TEST_CASE("Convenience exception macros") {
        SUBCASE("THROW_RUNTIME") {
            CHECK_THROWS_AS(THROW_RUNTIME("Runtime error"), std::runtime_error);
        }

        SUBCASE("THROW_LOGIC") {
            CHECK_THROWS_AS(THROW_LOGIC("Logic error"), std::logic_error);
        }

        SUBCASE("THROW_INVALID_ARG") {
            CHECK_THROWS_AS(THROW_INVALID_ARG("Invalid argument"), std::invalid_argument);
        }

        SUBCASE("THROW_OUT_OF_RANGE") {
            CHECK_THROWS_AS(THROW_OUT_OF_RANGE("Index out of range"), std::out_of_range);
        }

        SUBCASE("THROW_LENGTH") {
            CHECK_THROWS_AS(THROW_LENGTH("Length error"), std::length_error);
        }

        SUBCASE("THROW_DOMAIN") {
            CHECK_THROWS_AS(THROW_DOMAIN("Domain error"), std::domain_error);
        }
    }

    TEST_CASE("Message formatting with various types") {
        using namespace failsafe::detail;
        
        SUBCASE("Multiple argument types") {
            try {
                std::string str = "test";
                THROW(std::runtime_error, "String: ", str, ", Int: ", 42, ", Bool: ", true);
                FAIL("Exception should have been thrown");
            } catch (const std::runtime_error& e) {
                std::string msg(e.what());
                CHECK(msg.find("String:  test , Int:  42 , Bool:  true") != std::string::npos);
            }
        }

        SUBCASE("With formatters") {
            try {
                THROW(std::runtime_error, "Hex: ", hex(255), ", Bin: ", bin(15));
                FAIL("Exception should have been thrown");
            } catch (const std::runtime_error& e) {
                std::string msg(e.what());
                CHECK(msg.find("Hex:  0xff , Bin:  0b1111") != std::string::npos);
            }
        }

        SUBCASE("With containers") {
            try {
                std::vector<int> vec{1, 2, 3};
                THROW(std::runtime_error, "Vector: ", container(vec));
                FAIL("Exception should have been thrown");
            } catch (const std::runtime_error& e) {
                std::string msg(e.what());
                CHECK(msg.find("Vector:  [1, 2, 3]") != std::string::npos);
            }
        }

        SUBCASE("Empty message") {
            try {
                THROW(std::runtime_error, "");
                FAIL("Exception should have been thrown");
            } catch (const std::runtime_error& e) {
                std::string msg(e.what());
                CHECK(msg.find("test_exception.cc:") != std::string::npos);
                CHECK(msg.find("] ") != std::string::npos);
            }
        }
    }

    TEST_CASE("File and line information") {
        int line = 0;
        try {
            line = __LINE__ + 1;
            THROW(std::runtime_error, "Test location");
            FAIL("Exception should have been thrown");
        } catch (const std::runtime_error& e) {
            std::string msg(e.what());
            CHECK(msg.find("test_exception.cc:" + std::to_string(line)) != std::string::npos);
        }
    }

    TEST_CASE("Complex expressions in macros") {
        SUBCASE("Expression in condition") {
            int x = 5, y = 10;
            CHECK_THROWS_AS(THROW_IF(x < y && y > 0, std::runtime_error, "x < y"), 
                          std::runtime_error);
        }

        SUBCASE("Expression in message") {
            try {
                int a = 3, b = 4;
                THROW(std::runtime_error, "Sum: ", (a + b), ", Product: ", (a * b));
                FAIL("Exception should have been thrown");
            } catch (const std::runtime_error& e) {
                std::string msg(e.what());
                CHECK(msg.find("Sum:  7 , Product:  12") != std::string::npos);
            }
        }
    }
}

} // namespace custom_default_tests

// Restore original default exception
#undef FAILSAFE_DEFAULT_EXCEPTION
#define FAILSAFE_DEFAULT_EXCEPTION std::runtime_error

TEST_SUITE("Exception Macros - Default std::runtime_error") {
    TEST_CASE("THROW_DEFAULT with std::runtime_error") {
        CHECK_THROWS_AS(THROW_DEFAULT("Runtime default"), std::runtime_error);
    }
}