#include <failsafe/detail/string_utils.hh>
#include "strings_helper.hh"
#include <doctest/doctest.h>

#include <filesystem>
#include <chrono>
#include <optional>
#include <variant>
#include <string>
#include <vector>
#include <utility>
#include <tuple>

namespace fs = std::filesystem;
using namespace std::chrono_literals;


TEST_SUITE("string_utils") {
    using namespace failsafe::detail;

    TEST_CASE("build_message basic types") {
        SUBCASE("empty message") {
            CHECK(build_message() == "");
        }

        SUBCASE("single string") {
            CHECK(build_message("hello") == "hello");
        }

        SUBCASE("multiple strings") {
            CHECK(build_message("hello", "world") == "hello world");
        }

        SUBCASE("mixed basic types") {
            CHECK(build_message("Count:", 42, "Value:", 3.14) == "Count: 42 Value: 3.14");
        }

        SUBCASE("boolean values") {
            CHECK(build_message("Success:", true, "Failed:", false) == "Success: true Failed: false");
        }

        SUBCASE("nullptr handling") {
            void* ptr = nullptr;
            CHECK(build_message("Pointer:", ptr) == "Pointer: nullptr");
        }

        SUBCASE("valid pointer") {
            int value = 42;
            int* ptr = &value;
            std::string result = build_message("Pointer:", ptr);
            CHECK(::starts_with(result, "Pointer: 0x"));
        }
    }

    TEST_CASE("build_message with filesystem::path") {
        SUBCASE("simple path") {
            fs::path path("/home/user/file.txt");
            CHECK(build_message("Path:", path) == "Path: /home/user/file.txt");
        }

        SUBCASE("relative path") {
            fs::path path("../directory/file.txt");
            CHECK(build_message("Relative:", path) == "Relative: ../directory/file.txt");
        }

        SUBCASE("empty path") {
            fs::path path;
            CHECK(build_message("Empty:", path) == "Empty: ");
        }

        SUBCASE("path with spaces") {
            fs::path path("/home/user/my documents/file.txt");
            CHECK(build_message("Path:", path) == "Path: /home/user/my documents/file.txt");
        }
    }

    TEST_CASE("build_message with chrono durations") {
        SUBCASE("nanoseconds") {
            auto duration = 500ns;
            CHECK(build_message("Duration:", duration) == "Duration: 500ns");
        }

        SUBCASE("microseconds") {
            auto duration = 250us;
            CHECK(build_message("Duration:", duration) == "Duration: 250us");
        }

        SUBCASE("milliseconds") {
            auto duration = 100ms;
            CHECK(build_message("Duration:", duration) == "Duration: 100ms");
        }

        SUBCASE("seconds") {
            auto duration = 5s;
            CHECK(build_message("Duration:", duration) == "Duration: 5s");
        }

        SUBCASE("minutes") {
            auto duration = 2min;
            CHECK(build_message("Duration:", duration) == "Duration: 2min");
        }

        SUBCASE("hours") {
            auto duration = 3h;
            CHECK(build_message("Duration:", duration) == "Duration: 3h");
        }

        SUBCASE("mixed durations") {
            CHECK(build_message("Times:", 100ms, 5s, 2min) == "Times: 100ms 5s 2min");
        }
    }

    TEST_CASE("build_message with chrono time_point") {
        SUBCASE("system clock time point") {
            // Create a known time point (Unix epoch)
            auto tp = std::chrono::system_clock::time_point{};
            std::string result = build_message("Time:", tp);
            // Should be formatted as ISO 8601 with milliseconds
            CHECK(result == "Time: 1970-01-01T00:00:00.000Z");
        }

        SUBCASE("current time") {
            auto now = std::chrono::system_clock::now();
            std::string result = build_message("Now:", now);
            // Check format: "Now: YYYY-MM-DDTHH:MM:SS.mmmZ"
            CHECK(::starts_with(result, "Now: "));
            CHECK(result.find('T') != std::string::npos);
            CHECK(::ends_with(result, 'Z'));
            CHECK(result.find('.') != std::string::npos);
        }

        SUBCASE("steady clock time point") {
            auto tp = std::chrono::steady_clock::now();
            std::string result = build_message("Steady:", tp);
            // Should show duration since epoch
            CHECK(result.find("since epoch") != std::string::npos);
        }
    }

    TEST_CASE("build_message with optional") {
        SUBCASE("optional with value") {
            std::optional <int> opt = 42;
            CHECK(build_message("Optional:", opt) == "Optional: 42");
        }

        SUBCASE("empty optional") {
            std::optional <int> opt;
            CHECK(build_message("Optional:", opt) == "Optional: nullopt");
        }

        SUBCASE("optional string") {
            std::optional <std::string> opt = "hello";
            CHECK(build_message("Optional:", opt) == "Optional: hello");
        }

        SUBCASE("optional path") {
            std::optional <fs::path> opt = fs::path("/tmp/file.txt");
            CHECK(build_message("Optional path:", opt) == "Optional path: /tmp/file.txt");
        }

        SUBCASE("nested optional") {
            std::optional <std::optional <int>> opt = std::optional <int>(42);
            CHECK(build_message("Nested:", opt) == "Nested: 42");
        }

        SUBCASE("empty nested optional") {
            std::optional <std::optional <int>> opt = std::optional <int>{};
            CHECK(build_message("Nested:", opt) == "Nested: nullopt");
        }
    }

    TEST_CASE("build_message with variant") {
        using var_t = std::variant <int, std::string, double>;

        SUBCASE("variant with int") {
            var_t var = 42;
            CHECK(build_message("Variant:", var) == "Variant: 42");
        }

        SUBCASE("variant with string") {
            var_t var = std::string("hello");
            CHECK(build_message("Variant:", var) == "Variant: hello");
        }

        SUBCASE("variant with double") {
            var_t var = 3.14;
            CHECK(build_message("Variant:", var) == "Variant: 3.14");
        }

        SUBCASE("variant with monostate") {
            std::variant <std::monostate, int, std::string> var;
            CHECK(build_message("Variant:", var) == "Variant: monostate");
        }

        SUBCASE("variant with filesystem path") {
            std::variant <int, fs::path> var = fs::path("/home/user");
            CHECK(build_message("Variant:", var) == "Variant: /home/user");
        }

        SUBCASE("variant with chrono duration") {
            std::variant <int, std::chrono::milliseconds> var = 500ms;
            CHECK(build_message("Variant:", var) == "Variant: 500ms");
        }
    }

    TEST_CASE("build_message complex combinations") {
        SUBCASE("all types together") {
            fs::path path("/tmp/log.txt");
            std::optional <int> opt = 42;
            std::variant <std::string, int> var = "active";
            auto duration = 250ms;

            std::string result = build_message(
                "Path:", path,
                "Opt:", opt,
                "Var:", var,
                "Time:", duration,
                "Bool:", true
            );

            CHECK(result == "Path: /tmp/log.txt Opt: 42 Var: active Time: 250ms Bool: true");
        }

        SUBCASE("nested complex types") {
            std::optional <fs::path> opt_path = fs::path("/home");
            std::variant <std::optional <int>, std::string> var = std::optional <int>(100);

            std::string result = build_message("Complex:", opt_path, var);
            CHECK(result == "Complex: /home 100");
        }

        SUBCASE("vector of paths using loop") {
            std::vector <fs::path> paths = {
                "/home/user",
                "/tmp",
                "/var/log"
            };

            std::ostringstream oss;
            oss << "Paths:";
            for (const auto& p : paths) {
                append_to_stream(oss, " ");
                append_to_stream(oss, p);
            }

            CHECK(oss.str() == "Paths: /home/user /tmp /var/log");
        }
    }

    TEST_CASE("append_to_stream direct usage") {
        std::ostringstream oss;

        SUBCASE("multiple appends") {
            append_to_stream(oss, "Count:");
            append_to_stream(oss, " ");
            append_to_stream(oss, 42);
            append_to_stream(oss, " ");
            append_to_stream(oss, true);

            CHECK(oss.str() == "Count: 42 true");
        }

        SUBCASE("complex type appends") {
            fs::path path("/etc/config");
            std::optional <std::string> opt = "enabled";
            auto duration = 1s;

            append_to_stream(oss, path);
            append_to_stream(oss, " - ");
            append_to_stream(oss, opt);
            append_to_stream(oss, " (");
            append_to_stream(oss, duration);
            append_to_stream(oss, ")");

            CHECK(oss.str() == "/etc/config - enabled (1s)");
        }
    }

    TEST_CASE("build_message with pair") {
        SUBCASE("basic pair") {
            auto p = std::make_pair(42, "test");
            CHECK(build_message("Pair:", p) == "Pair: (42, test)");
        }

        SUBCASE("pair with different types") {
            auto p = std::make_pair(3.14, true);
            CHECK(build_message(p) == "(3.14, true)");
        }

        SUBCASE("nested pair") {
            auto p = std::make_pair(1, std::make_pair(2.5, "nested"));
            CHECK(build_message(p) == "(1, (2.5, nested))");
        }

        SUBCASE("pair with optional") {
            auto p = std::make_pair(std::optional <int>(42), std::optional <int>());
            CHECK(build_message(p) == "(42, nullopt)");
        }

        SUBCASE("pair with filesystem path") {
            auto p = std::make_pair(fs::path("/tmp"), 123);
            CHECK(build_message(p) == "(/tmp, 123)");
        }

        SUBCASE("pair with chrono duration") {
            auto p = std::make_pair(100ms, "timeout");
            CHECK(build_message(p) == "(100ms, timeout)");
        }
    }

    TEST_CASE("build_message with tuple") {
        SUBCASE("empty tuple") {
            auto t = std::make_tuple();
            CHECK(build_message(t) == "()");
        }

        SUBCASE("single element tuple") {
            auto t = std::make_tuple(42);
            CHECK(build_message(t) == "(42)");
        }

        SUBCASE("multiple element tuple") {
            auto t = std::make_tuple(1, 2.5, "three");
            CHECK(build_message(t) == "(1, 2.5, three)");
        }

        SUBCASE("nested tuple") {
            auto t = std::make_tuple(1, std::make_tuple(2, 3), 4);
            CHECK(build_message(t) == "(1, (2, 3), 4)");
        }

        SUBCASE("large tuple") {
            auto t = std::make_tuple(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
            CHECK(build_message(t) == "(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)");
        }

        SUBCASE("tuple with complex types") {
            auto t = std::make_tuple(
                fs::path("/home"),
                std::optional <int>(42),
                100ms,
                true
            );
            CHECK(build_message(t) == "(/home, 42, 100ms, true)");
        }
    }

    TEST_CASE("build_message with mixed pair/tuple") {
        SUBCASE("tuple containing pair") {
            auto tp = std::make_tuple(1, std::make_pair(2.5, "pair"), true);
            CHECK(build_message(tp) == "(1, (2.5, pair), true)");
        }

        SUBCASE("pair containing tuple") {
            auto pt = std::make_pair(std::make_tuple(1, 2, 3), "tuple");
            CHECK(build_message(pt) == "((1, 2, 3), tuple)");
        }

        SUBCASE("variant with pair") {
            using var_t = std::variant <int, std::pair <double, bool>>;
            var_t v = std::make_pair(3.14, false);
            CHECK(build_message(v) == "(3.14, false)");
        }

        SUBCASE("optional pair") {
            std::optional <std::pair <int, std::string>> opt = std::make_pair(42, std::string("test"));
            CHECK(build_message(opt) == "(42, test)");

            std::optional <std::pair <int, std::string>> empty;
            CHECK(build_message(empty) == "nullopt");
        }

        SUBCASE("complex nested structure") {
            auto complex = std::make_tuple(
                std::make_pair(1, "first"),
                std::make_tuple(2, 3),
                std::optional <std::pair <int, int>>(std::make_pair(4, 5))
            );
            CHECK(build_message(complex) == "((1, first), (2, 3), (4, 5))");
        }
    }

    TEST_CASE("build_message with containers") {
        SUBCASE("basic containers") {
            std::vector <int> vec = {1, 2, 3, 4, 5};
            CHECK(build_message(vec) == "[1, 2, 3, 4, 5]");

            std::list <std::string> lst = {"hello", "world"};
            CHECK(build_message(lst) == "[hello, world]");

            std::deque <double> deq = {1.1, 2.2};
            CHECK(build_message(deq) == "[1.1, 2.2]");

            std::array <int, 3> arr = {10, 20, 30};
            CHECK(build_message(arr) == "[10, 20, 30]");
        }

        SUBCASE("set containers") {
            std::set <int> s = {3, 1, 2};
            CHECK(build_message(s) == "{1, 2, 3}");

            std::unordered_set <int> us = {1, 2, 3};
            // Order may vary for unordered_set, just check it starts with {
            auto result = build_message(us);
            CHECK(::starts_with(result, "{"));
            CHECK(::ends_with(result, "}"));
            CHECK(result.find("1") != std::string::npos);
            CHECK(result.find("2") != std::string::npos);
            CHECK(result.find("3") != std::string::npos);
        }

        SUBCASE("map containers") {
            std::map <std::string, int> m = {{"one", 1}, {"two", 2}};
            CHECK(build_message(m) == "{one: 1, two: 2}");

            std::map <int, std::string> m2 = {{1, "first"}, {2, "second"}};
            CHECK(build_message(m2) == "{1: first, 2: second}");
        }

        SUBCASE("empty containers") {
            std::vector <int> vec;
            CHECK(build_message(vec) == "[]");

            std::set <int> s;
            CHECK(build_message(s) == "{}");

            std::map <int, int> m;
            CHECK(build_message(m) == "{}");
        }

        SUBCASE("nested containers") {
            std::vector <std::vector <int>> matrix = {{1, 2}, {3, 4}};
            CHECK(build_message(matrix) == "[[1, 2], [3, 4]]");

            std::map <std::string, std::vector <int>> data = {{"a", {1, 2}}, {"b", {3}}};
            CHECK(build_message(data) == "{a: [1, 2], b: [3]}");
        }

        SUBCASE("containers with special types") {
            std::vector <std::optional <int>> opts = {42, std::nullopt, 100};
            CHECK(build_message(opts) == "[42, nullopt, 100]");

            std::vector <std::pair <int, std::string>> pairs = {{1, "one"}, {2, "two"}};
            CHECK(build_message(pairs) == "[(1, one), (2, two)]");
        }
    }

    TEST_CASE("container formatter") {
        std::vector <int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        SUBCASE("basic limiting") {
            CHECK(build_message(container(vec, 3)) == "[1, 2, 3, ...]");
            CHECK(build_message(container(vec, 5)) == "[1, 2, 3, 4, 5, ...]");
            CHECK(build_message(container(vec, 10)) == "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]");
            CHECK(build_message(container(vec, 20)) == "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]");
        }

        SUBCASE("custom options") {
            auto fmt = container(vec, [](auto& f) {
                f.max_items = 3;
                f.prefix = "{";
                f.suffix = "}";
                f.delimiter = "; ";
            });
            CHECK(build_message(fmt) == "{1; 2; 3; ...}");
        }

        SUBCASE("with indices") {
            auto fmt = container(vec, [](auto& f) {
                f.max_items = 3;
                f.show_indices = true;
            });
            CHECK(build_message(fmt) == "[[0]: 1, [1]: 2, [2]: 3, ...]");
        }

        SUBCASE("starting index") {
            auto fmt = container(vec, [](auto& f) {
                f.start_index = 5;
                f.max_items = 3;
            });
            CHECK(build_message(fmt) == "[6, 7, 8, ...]");
        }

        SUBCASE("out of bounds start index") {
            auto fmt = container(vec, [](auto& f) {
                f.start_index = 20;
                f.max_items = 3;
            });
            CHECK(build_message(fmt) == "[]");
        }

        SUBCASE("custom ellipsis") {
            auto fmt = container(vec, [](auto& f) {
                f.max_items = 2;
                f.ellipsis = "and more";
            });
            CHECK(build_message(fmt) == "[1, 2, and more]");
        }

        SUBCASE("multiline format") {
            auto fmt = container(vec, [](auto& f) {
                f.max_items = 3;
                f.multiline = true;
                f.indent = "  ";
            });
            CHECK(build_message(fmt) == "[\n  1, \n  2, \n  3, \n  ...\n]");
        }

        SUBCASE("empty container with formatter") {
            std::vector <int> empty;
            auto fmt = container(empty, 5);
            CHECK(build_message(fmt) == "[]");
        }
    }

    TEST_CASE("container formatter with maps") {
        std::map <std::string, int> m = {{"a", 1}, {"b", 2}, {"c", 3}, {"d", 4}};

        SUBCASE("map limiting") {
            CHECK(build_message(container(m, 2)) == "[a: 1, b: 2, ...]");
        }

        SUBCASE("map with custom format") {
            auto fmt = container(m, [](auto& f) {
                f.max_items = 2;
                f.prefix = "Map{";
                f.suffix = "}";
                f.delimiter = ", ";
            });
            CHECK(build_message(fmt) == "Map{a: 1, b: 2, ...}");
        }
    }

    TEST_CASE("edge cases") {
        SUBCASE("empty strings") {
            CHECK(build_message("", "", "") == "  ");
        }

        SUBCASE("single space preservation") {
            CHECK(build_message("a", " ", "b") == "a   b");
        }

        SUBCASE("large numbers") {
            CHECK(build_message("Large:", 9223372036854775807LL) == "Large: 9223372036854775807");
        }

        SUBCASE("special characters") {
            CHECK(build_message("Special:", "line1\nline2\ttab") == "Special: line1\nline2\ttab");
        }

        SUBCASE("empty optional of optional") {
            std::optional <std::optional <std::string>> opt;
            CHECK(build_message("Empty:", opt) == "Empty: nullopt");
        }
    }
}
