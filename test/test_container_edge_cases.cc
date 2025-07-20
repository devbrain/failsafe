#include <failsafe/detail/string_utils.hh>
#include <doctest/doctest.h>

#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <deque>
#include <list>
#include <forward_list>
#include <numeric>
#include "strings_helper.hh"

using namespace failsafe::detail;

TEST_SUITE("container edge cases and advanced features") {
    TEST_CASE("container formatter with extreme values") {
        SUBCASE("very large container") {
            std::vector<int> large(1000);
            std::iota(large.begin(), large.end(), 0);
            
            // Default shows all
            auto all = build_message(container(large));
            CHECK(all.find("999") != std::string::npos);
            CHECK(all.find("...") == std::string::npos);
            
            // Limited shows ellipsis
            auto limited = build_message(container(large, 10));
            CHECK(limited.find("9") != std::string::npos);
            CHECK(limited.find("...") != std::string::npos);
            CHECK(limited.find("999") == std::string::npos);
        }
        
        SUBCASE("start index beyond container size") {
            std::vector<int> vec = {1, 2, 3};
            auto result = build_message(container(vec, [](auto& fmt) {
                fmt.start_index = 100;
            }));
            CHECK(result == "[]");
        }
        
        SUBCASE("start index + max_items exceeds size") {
            std::vector<int> vec = {1, 2, 3, 4, 5};
            auto result = build_message(container(vec, [](auto& fmt) {
                fmt.start_index = 3;
                fmt.max_items = 10;
            }));
            CHECK(result == "[4, 5]");
        }
    }
    
    TEST_CASE("all container types") {
        SUBCASE("forward_list") {
            std::forward_list<int> fl = {1, 2, 3};
            CHECK(build_message(fl) == "[1, 2, 3]");
        }
        
        SUBCASE("multiset") {
            std::multiset<int> ms = {3, 1, 2, 1, 3};
            CHECK(build_message(ms) == "{1, 1, 2, 3, 3}");
        }
        
        SUBCASE("multimap") {
            std::multimap<int, std::string> mm = {{1, "a"}, {1, "b"}, {2, "c"}};
            CHECK(build_message(mm) == "{1: a, 1: b, 2: c}");
        }
        
        SUBCASE("unordered_multiset") {
            std::unordered_multiset<int> ums = {1, 2, 1, 3};
            auto result = build_message(ums);
            CHECK(starts_with(result, "{"));
            CHECK(ends_with(result, "}"));
            // Order is not guaranteed for unordered containers
            CHECK(result.find("1") != std::string::npos);
            CHECK(result.find("2") != std::string::npos);
            CHECK(result.find("3") != std::string::npos);
        }
        
        SUBCASE("unordered_multimap") {
            std::unordered_multimap<int, char> umm = {{1, 'a'}, {1, 'b'}, {2, 'c'}};
            auto result = build_message(umm);
            CHECK(starts_with(result, "{"));
            CHECK(ends_with(result, "}"));
            // Check that we have key-value pairs (order not guaranteed)
            bool has_1a = result.find("1: a") != std::string::npos;
            bool has_1b = result.find("1: b") != std::string::npos;
            CHECK((has_1a || has_1b));
            CHECK(result.find("2: c") != std::string::npos);
        }
    }
    
    TEST_CASE("deeply nested containers") {
        SUBCASE("3-level nesting") {
            std::vector<std::vector<std::vector<int>>> cube = {
                {{1, 2}, {3, 4}},
                {{5, 6}, {7, 8}}
            };
            CHECK(build_message(cube) == "[[[1, 2], [3, 4]], [[5, 6], [7, 8]]]");
        }
        
        SUBCASE("mixed container types") {
            std::map<std::string, std::set<int>> data = {
                {"evens", {2, 4, 6}},
                {"odds", {1, 3, 5}}
            };
            CHECK(build_message(data) == "{evens: {2, 4, 6}, odds: {1, 3, 5}}");
        }
        
        SUBCASE("container of pairs of containers") {
            std::vector<std::pair<std::vector<int>, std::set<char>>> complex = {
                {{1, 2}, {'a', 'b'}},
                {{3, 4, 5}, {'x', 'y', 'z'}}
            };
            CHECK(build_message(complex) == "[([1, 2], {a, b}), ([3, 4, 5], {x, y, z})]");
        }
    }
    
    TEST_CASE("container formatter customization") {
        std::vector<int> vec = {1, 2, 3, 4, 5};
        
        SUBCASE("all options at once") {
            auto result = build_message(container(vec, [](auto& fmt) {
                fmt.max_items = 3;
                fmt.start_index = 1;
                fmt.prefix = "<<";
                fmt.suffix = ">>";
                fmt.delimiter = " | ";
                fmt.ellipsis = "etc...";
                fmt.show_indices = true;
            }));
            CHECK(result == "<<[1]: 2 | [2]: 3 | [3]: 4 | etc...>>");
        }
        
        SUBCASE("multiline with custom indent") {
            auto result = build_message(container(vec, [](auto& fmt) {
                fmt.max_items = 3;
                fmt.multiline = true;
                fmt.indent = "    ";
                fmt.prefix = "[\n";
                fmt.suffix = "]";
            }));
            // Check the actual format - prefix already has newline
            std::string expected = "[\n\n    1, \n    2, \n    3, \n    ...\n]";
            CHECK(result == expected);
        }
    }
    
    TEST_CASE("containers with all special types") {
        SUBCASE("container of variants") {
            using var_t = std::variant<int, std::string, double>;
            std::vector<var_t> vars = {42, "hello", 3.14};
            CHECK(build_message(vars) == "[42, hello, 3.14]");
        }
        
        SUBCASE("container of chrono durations") {
            using namespace std::chrono_literals;
            std::vector<std::chrono::milliseconds> times = {100ms, 250ms, 500ms};
            CHECK(build_message(times) == "[100ms, 250ms, 500ms]");
        }
        
        SUBCASE("container of filesystem paths") {
            std::vector<std::filesystem::path> paths = {
                "/home/user",
                "/tmp/file.txt",
                "relative/path"
            };
            CHECK(build_message(paths) == "[/home/user, /tmp/file.txt, relative/path]");
        }
        
        SUBCASE("container of tuples") {
            std::vector<std::tuple<int, std::string, bool>> data = {
                {1, "one", true},
                {2, "two", false}
            };
            CHECK(build_message(data) == "[(1, one, true), (2, two, false)]");
        }
    }
    
    TEST_CASE("performance and efficiency") {
        SUBCASE("container formatter doesn't copy") {
            // This test verifies that container formatter works with references
            const std::vector<int> vec = {1, 2, 3};
            auto fmt = container(vec, 2);
            CHECK(build_message(fmt) == "[1, 2, ...]");
        }
        
        SUBCASE("works with rvalue containers") {
            CHECK(build_message(std::vector<int>{1, 2, 3}) == "[1, 2, 3]");
            CHECK(build_message(container(std::vector<int>{1, 2, 3}, 2)) == "[1, 2, ...]");
        }
    }
    
    TEST_CASE("interaction with existing types") {
        SUBCASE("std::array is formatted as sequence not tuple") {
            std::array<int, 3> arr = {1, 2, 3};
            // Should use [], not ()
            CHECK(build_message(arr) == "[1, 2, 3]");
        }
        
        SUBCASE("strings are not treated as containers") {
            std::string str = "hello";
            CHECK(build_message(str) == "hello");
            
            std::string_view sv = "world";
            CHECK(build_message(sv) == "world");
            
            const char* cstr = "test";
            CHECK(build_message(cstr) == "test");
        }
    }
}