#include <doctest/doctest.h>
#include <failsafe/enforce.hh>
#include <failsafe/expected.hh>
#include <failsafe/message.hh>

#include <cstdint>
#include <limits>
#include <optional>
#include <tuple>
#include <variant>
#include <vector>

enum class draw_style { normal, italic };

namespace enum_tests {
    enum plain : int { ready = 1, failed = 2 };
    enum class status { ready, failed };
    enum class byte : unsigned char { ready = 1 };
    enum class signed_byte : signed char { negative = -1 };
    enum class wide : std::uint64_t { maximum = UINT64_MAX };
    enum class outside_range { value = 1000 };
    enum class custom_range { value = 500 };
    enum class empty : int {};
    enum class streamable { value = 1 };

    inline std::ostream& operator<<(std::ostream& stream, streamable) {
        return stream << "custom enum text";
    }
}

#if defined(FAILSAFE_USE_MAGIC_ENUM) && FAILSAFE_USE_MAGIC_ENUM
template <>
struct magic_enum::customize::enum_range<enum_tests::custom_range> {
    static constexpr int min = 490;
    static constexpr int max = 510;
};

TEST_SUITE("Enum formatting with magic_enum") {
    using failsafe::build_message;
    using namespace enum_tests;

    TEST_CASE("Scoped and unscoped enums use names") {
        const status state = status::ready;
        CHECK(build_message("State:", state, status::failed) == "State: ready failed");
        CHECK(build_message(ready, failed) == "ready failed");
        CHECK(build_message(byte::ready, signed_byte::negative) == "ready negative");
        CHECK(build_message(streamable::value) == "value");
    }

    TEST_CASE("Unknown and unreflected values use the underlying number") {
        CHECK(build_message(static_cast<status>(99)) == "99");
        CHECK(build_message(static_cast<byte>(200)) == "200");
        CHECK(build_message(static_cast<signed_byte>(-100)) == "-100");
        CHECK(build_message(wide::maximum) == std::to_string(std::numeric_limits<std::uint64_t>::max()));
        CHECK(build_message(outside_range::value) == "1000");
        CHECK(build_message(static_cast<empty>(7)) == "7");
    }

    TEST_CASE("User reflection ranges are respected") {
        CHECK(build_message(custom_range::value) == "value");
        CHECK(build_message(static_cast<custom_range>(501)) == "501");
    }

    TEST_CASE("Enum names propagate through compound formatters") {
        const std::vector<status> values{status::ready, status::failed};
        CHECK(build_message(values) == "[ready, failed]");
        CHECK(build_message(failsafe::detail::container(values, 1)) == "[ready, ...]");
        CHECK(build_message(std::optional<status>{status::ready}) == "ready");
        CHECK(build_message(std::variant<status, int>{status::failed}) == "failed");
        CHECK(build_message(std::make_tuple(status::ready, status::failed)) == "(ready, failed)");
        CHECK(build_message(failsafe::detail::uppercase(status::ready)) == "READY");
    }

    TEST_CASE("Enforcement and exception messages accept enums") {
        CHECK_THROWS_WITH_AS(ENFORCE(false, "State:", status::failed),
                             doctest::Contains("State: failed"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE(false)("State:", status::failed),
                             doctest::Contains("State: failed"), std::runtime_error);
        CHECK_THROWS_WITH_AS(THROW(std::runtime_error, "State:", status::failed),
                             doctest::Contains("State: failed"), std::runtime_error);
        failsafe::expected<int, status> result = failsafe::make_unexpected(status::failed);
        CHECK_THROWS_WITH_AS(ENFORCE(result), doctest::Contains("failed"), std::runtime_error);
    }
}
#else
TEST_SUITE("Enum formatting without magic_enum") {
    using failsafe::build_message;
    using namespace enum_tests;

    TEST_CASE("Scoped and unscoped enums include their type and number") {
        CHECK(build_message(draw_style::normal) == "(draw_style:0)");
        CHECK(build_message(static_cast<draw_style>(42)) == "(draw_style:42)");
        const status state = status::ready;
        CHECK(build_message("State:", state) == "State: (enum_tests::status:0)");
        CHECK(build_message(ready, failed) == "(enum_tests::plain:1) (enum_tests::plain:2)");
        CHECK(build_message(streamable::value) == "(enum_tests::streamable:1)");
    }

    TEST_CASE("Underlying values retain their sign and width") {
        CHECK(build_message(byte::ready) == "(enum_tests::byte:1)");
        CHECK(build_message(static_cast<byte>(200)) == "(enum_tests::byte:200)");
        CHECK(build_message(signed_byte::negative) == "(enum_tests::signed_byte:-1)");
        CHECK(build_message(static_cast<signed_byte>(-100)) == "(enum_tests::signed_byte:-100)");
        CHECK(build_message(wide::maximum) == "(enum_tests::wide:18446744073709551615)");
        CHECK(build_message(outside_range::value) == "(enum_tests::outside_range:1000)");
        CHECK(build_message(static_cast<empty>(7)) == "(enum_tests::empty:7)");
    }

    TEST_CASE("Type and number propagate through compound formatters") {
        const std::vector<status> values{status::ready, status::failed};
        CHECK(build_message(values) == "[(enum_tests::status:0), (enum_tests::status:1)]");
        CHECK(build_message(failsafe::detail::container(values, 1)) == "[(enum_tests::status:0), ...]");
        CHECK(build_message(std::optional<status>{status::ready}) == "(enum_tests::status:0)");
        CHECK(build_message(std::variant<status, int>{status::failed}) == "(enum_tests::status:1)");
        CHECK(build_message(std::make_tuple(status::ready, status::failed)) ==
              "((enum_tests::status:0), (enum_tests::status:1))");
        CHECK(build_message(failsafe::detail::uppercase(status::ready)) == "(ENUM_TESTS::STATUS:0)");
    }

    TEST_CASE("Enforcement and exception messages accept enums") {
        CHECK_THROWS_WITH_AS(ENFORCE(false, "State:", status::failed),
                             doctest::Contains("State: (enum_tests::status:1)"), std::runtime_error);
        CHECK_THROWS_WITH_AS(ENFORCE(false)("State:", status::failed),
                             doctest::Contains("State: (enum_tests::status:1)"), std::runtime_error);
        CHECK_THROWS_WITH_AS(THROW(std::runtime_error, "State:", status::failed),
                             doctest::Contains("State: (enum_tests::status:1)"), std::runtime_error);
        failsafe::expected<int, status> result = failsafe::make_unexpected(status::failed);
        CHECK_THROWS_WITH_AS(ENFORCE(result), doctest::Contains("(enum_tests::status:1)"), std::runtime_error);
    }
}
#endif
