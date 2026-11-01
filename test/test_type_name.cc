// Keep this first to check that the public header is self-contained.
#include <failsafe/type_name.hh>

#include <doctest/doctest.h>
#include <failsafe/enforce.hh>
#include <failsafe/message.hh>

#include <sstream>
#include <type_traits>
#include <vector>

namespace type_name_tests {
    struct widget;
    enum class state { ready, failed };
    template<typename T> struct wrapper;

    // Compiler spellings differ in whitespace, including before pointers/arrays.
    constexpr bool matches(std::string_view actual, std::string_view expected) {
        std::size_t index = 0;
        for (const char character : actual) {
            if (character == ' ') continue;
            if (index == expected.size() || character != expected[index++]) return false;
        }
        return index == expected.size();
    }
}

using failsafe::type_name_v;

static_assert(type_name_v<void> == "void");
static_assert(type_name_v<int> == "int");
static_assert(type_name_v<bool> == "bool");
static_assert(failsafe::type_name<int>::value == type_name_v<int>);
static_assert(failsafe::type_name<int>{}() == type_name_v<int>);
static_assert(std::string_view(failsafe::type_name<int>{}) == "int");
static_assert(std::is_same_v<decltype(type_name_v<int>), const std::string_view>);
static_assert(type_name_tests::matches(type_name_v<const volatile int&>, "constvolatileint&") ||
              type_name_tests::matches(type_name_v<const volatile int&>, "intconstvolatile&"));
static_assert(type_name_tests::matches(type_name_v<int&&>, "int&&"));
static_assert(type_name_tests::matches(type_name_v<int* const>, "int*const"));
static_assert(type_name_tests::matches(type_name_v<int[3][4]>, "int[3][4]"));
static_assert(type_name_tests::matches(type_name_v<const int(&)[3]>, "constint(&)[3]") ||
              type_name_tests::matches(type_name_v<const int(&)[3]>, "intconst(&)[3]"));
static_assert(type_name_v<type_name_tests::widget>.find("type_name_tests::widget") != std::string_view::npos);
static_assert(type_name_v<type_name_tests::state>.find("type_name_tests::state") != std::string_view::npos);
static_assert(type_name_v<type_name_tests::wrapper<int[3]>>.find("[3]") != std::string_view::npos);
static_assert(type_name_v<std::vector<int>>.find("vector") != std::string_view::npos);
// MSVC adds struct keywords and spaces between nested closing brackets. Use the
// same whitespace-insensitive comparison as for pointers and arrays, while still
// checking that both nested types are present and fully closed.
static_assert(type_name_tests::matches(
                  type_name_v<failsafe::type_name<failsafe::type_name<int>>>,
                  "failsafe::type_name<failsafe::type_name<int>>") ||
              type_name_tests::matches(
                  type_name_v<failsafe::type_name<failsafe::type_name<int>>>,
                  "structfailsafe::type_name<structfailsafe::type_name<int>>"));
static_assert(type_name_v<int(double)>.find("double") != std::string_view::npos);
static_assert(type_name_v<int(double)>.back() == ')');
static_assert(failsafe::type_hash_v<int> == 340908721U); // BSW compatibility.
static_assert(failsafe::detail::type_name_crc32("123456789") == 0xcbf43926U); // CRC32 check value.
static_assert(failsafe::detail::type_name_crc32("") == 0U);

TEST_SUITE("Compile-time type names") {
    TEST_CASE("Names and type-name objects can be printed") {
        std::ostringstream stream;
        stream << type_name_v<int> << ' ' << failsafe::type_name<bool>{};
        CHECK(stream.str() == "int bool");
        CHECK(failsafe::build_message("Type:", type_name_v<int>) == "Type: int");
        CHECK(failsafe::build_message("Type:", failsafe::type_name<bool>{}) == "Type: bool");
    }

    TEST_CASE("Type names work in enforcement and exceptions") {
        CHECK_THROWS_WITH_AS(ENFORCE(false, "Invalid type:", type_name_v<int>),
                             doctest::Contains("Invalid type: int"), std::runtime_error);
        CHECK_THROWS_WITH_AS(THROW(std::runtime_error, "Invalid type:", failsafe::type_name<bool>{}),
                             doctest::Contains("Invalid type: bool"), std::runtime_error);
    }

    TEST_CASE("Views retain their content and qualifiers") {
        constexpr auto name = [] { return type_name_v<const int(&)[3]>; }();
        CHECK((type_name_tests::matches(name, "constint(&)[3]") ||
               type_name_tests::matches(name, "intconst(&)[3]")));
        CHECK(type_name_v<int> != type_name_v<const int>);
        CHECK(type_name_v<int> != type_name_v<int&>);
    }
}
