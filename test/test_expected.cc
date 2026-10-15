#include <doctest/doctest.h>
#include <failsafe/expected.hh>
#include <failsafe/enforce.hh>
#include <string>

namespace {
    failsafe::expected<int, std::string> divide(int a, int b) {
        if (b == 0) {
            return failsafe::make_unexpectedf("Division by zero:", a, "/", b);
        }
        return a / b;
    }

    failsafe::result perform_action(bool succeed) {
        if (!succeed) {
            return failsafe::make_unexpectedf("Action failed with code", 404);
        }
        return {};
    }
}

TEST_SUITE("expected") {
    TEST_CASE("success and error cases") {
        auto ok = divide(10, 2);
        CHECK(ok.has_value());
        CHECK(static_cast<bool>(ok));
        CHECK(*ok == 5);
        CHECK(ok.value() == 5);

        auto err = divide(10, 0);
        CHECK_FALSE(err.has_value());
        CHECK_FALSE(static_cast<bool>(err));
        CHECK(err.error() == "Division by zero: 10 / 0");
    }

    TEST_CASE("make_unexpected with various types") {
        // String literal
        auto u1 = failsafe::make_unexpected("literal error");
        failsafe::expected<int, std::string> e1 = u1;
        CHECK_FALSE(e1.has_value());
        CHECK(e1.error() == "literal error");

        // std::string lvalue
        std::string msg = "dynamic error";
        auto u2 = failsafe::make_unexpected(msg);
        failsafe::expected<int, std::string> e2 = u2;
        CHECK(e2.error() == "dynamic error");

        // Explicit template argument with lvalue
        auto u3 = failsafe::make_unexpected<std::string>(msg);
        failsafe::expected<int, std::string> e3 = u3;
        CHECK(e3.error() == "dynamic error");

        // Int error code
        auto u4 = failsafe::make_unexpected(42);
        failsafe::expected<double, int> e4 = u4;
        CHECK(e4.error() == 42);
    }

    TEST_CASE("void result type") {
        auto ok = perform_action(true);
        CHECK(ok.has_value());

        auto err = perform_action(false);
        CHECK_FALSE(err.has_value());
        CHECK(err.error() == "Action failed with code 404");
    }

    TEST_CASE("expected implementation string") {
        const char* impl = failsafe::expected_implementation();
        CHECK(impl != nullptr);
        if (failsafe::using_std_expected) {
            CHECK(std::string(impl) == "std::expected");
        } else {
            CHECK(std::string(impl) == "tl::expected");
        }
    }

    TEST_CASE("ENFORCE integration with expected") {
        auto ok = divide(20, 4);
        int val = 0;
        CHECK_NOTHROW(val = ENFORCE(ok)("Failed to divide"));
        CHECK(val == 5);

        auto err = divide(20, 0);
        CHECK_THROWS_AS(ENFORCE(err)("Divide failed"), std::runtime_error);
    }
}
