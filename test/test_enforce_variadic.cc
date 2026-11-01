#include <doctest/doctest.h>
#include <failsafe/enforce.hh>
#include <failsafe/expected.hh>
#include <memory>
#include <string>

TEST_SUITE("Variadic enforcement") {
    TEST_CASE("Optional messages and default diagnostics") {
        CHECK_NOTHROW(ENFORCE(true));
        CHECK_NOTHROW(ENFORCE(true, "message", 42));
        CHECK_THROWS_WITH_AS(ENFORCE(false),
                            doctest::Contains("Expression must be true"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE(false, "custom", 42),
                            doctest::Contains("custom 42"), std::runtime_error);

        int* ptr = nullptr;
        CHECK_THROWS_WITH_AS(ENFORCE_NOT_NULL(ptr),
                            doctest::Contains("Null pointer: ptr"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE_NOT_NULL(ptr, "custom null"),
                            doctest::Contains("custom null"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE_VALID_INDEX(-1, 3u),
                            doctest::Contains("Index out of bounds:"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE_VALID_INDEX(-1, 3u, "custom index", -1),
                            doctest::Contains("custom index -1"), std::runtime_error);
    }

    TEST_CASE("All comparison and exception variants accept messages") {
        CHECK_THROWS_WITH_AS(ENFORCE_EQ(1, 2, "equal"), doctest::Contains("equal"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE_NE(1, 1, "different"), doctest::Contains("different"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE_LT(2, 1, "less"), doctest::Contains("less"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE_GT(1, 2, "greater"), doctest::Contains("greater"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE_LE(2, 1, "at most"), doctest::Contains("at most"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE_GE(1, 2, "at least"), doctest::Contains("at least"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE_IN_RANGE(4, 1, 3, "range", 4),
                            doctest::Contains("range 4"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE_THROW(false, std::logic_error, "logic", 42),
                            doctest::Contains("logic 42"), std::logic_error);
        CHECK_THROWS_AS(ENFORCE_THROW(false, std::logic_error), std::logic_error);
        CHECK_NOTHROW(ENFORCE_TRAP(true));
        CHECK_NOTHROW(ENFORCE_TRAP(true, "trap message"));
    }

    TEST_CASE("Checked operation precedes diagnostic arguments") {
        int calls = 0;
        int diagnostics = 0;
        std::string error = "stale";
        auto operation = [&](bool success) {
            ++calls;
            error = "current";
            return success;
        };
        auto diagnostic = [&] {
            ++diagnostics;
            CHECK(calls == diagnostics);
            return error;
        };
        CHECK_THROWS_WITH_AS(ENFORCE(operation(false), diagnostic()),
                            doctest::Contains("current"), std::runtime_error);
        CHECK_NOTHROW(ENFORCE(operation(true), diagnostic()));
        CHECK(calls == 2);
        CHECK(diagnostics == 2); // Preserve the existing eager argument evaluation.

        int value = 0;
        int bound = 0;
        CHECK_NOTHROW(ENFORCE_EQ(++value, ++bound, "once"));
        CHECK(value == 1);
        CHECK(bound == 1);
    }

    TEST_CASE("Value access and expected ownership are preserved") {
        int value = 42;
        int* ptr = ENFORCE(&value, "pointer");
        CHECK(ptr == &value);
        CHECK(ENFORCE_EQ(value, 42, "value").get() == 42);

        failsafe::expected<std::unique_ptr<int>, std::string> named = std::make_unique<int>(42);
        CHECK(ENFORCE(named, "named expected").get().get() == named->get());
        REQUIRE(*named != nullptr);
        CHECK(**named == 42);
        auto temporary = ENFORCE((failsafe::expected<std::unique_ptr<int>, std::string>(
            std::make_unique<int>(7))));
        CHECK(*temporary.get() == 7);
        CHECK(*ENFORCE((failsafe::expected<std::unique_ptr<int>, std::string>(
            std::make_unique<int>(9))), "temporary expected").get() == 9);

        failsafe::expected<int, std::string> failed = failsafe::make_unexpected("underlying error");
        CHECK_THROWS_WITH_AS(ENFORCE(failed),
                            doctest::Contains("underlying error"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE(failed, "custom expected error"),
                            doctest::Contains("custom expected error"), std::runtime_error);
    }

    TEST_CASE("Legacy message calls remain compatible during migration") {
        CHECK_THROWS_WITH_AS(ENFORCE(false)("legacy"), doctest::Contains("legacy"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE_GT(0, 1)("legacy comparison"),
                            doctest::Contains("legacy comparison"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE(false)("first")("second"), doctest::Contains("first"), std::runtime_error);
    }

    TEST_CASE("Debug checks retain release elision") {
        int checks = 0;
        int messages = 0;
        DEBUG_ENFORCE(++checks > 0, "debug", ++messages);
#ifdef NDEBUG
        CHECK(checks == 0);
        CHECK(messages == 0);
        DEBUG_ENFORCE(undefined_condition, undefined_message);
#else
        CHECK(checks == 1);
        CHECK(messages == 1);
#endif
    }

    TEST_CASE("Diagnostics retain the invocation location") {
        const int line = __LINE__ + 2;
        try {
            ENFORCE(false, "location");
            FAIL("Expected enforcement failure");
        } catch (const std::runtime_error& error) {
            const std::string message = error.what();
            CHECK(message.find(__FILE__) != std::string::npos);
            CHECK(message.find(":" + std::to_string(line) + "]") != std::string::npos);
        }
    }

    TEST_CASE("Nested enforcement and long messages") {
        CHECK(ENFORCE(ENFORCE(42, "inner").get(), "outer").get() == 42);
        CHECK_NOTHROW(ENFORCE(true,
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
            17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
            33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
            49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63));
    }
}
