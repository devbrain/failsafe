//
// Unit tests for location formatting
//

#include <doctest/doctest.h>
#include <failsafe/detail/location_format.hh>
#include <string>

using namespace failsafe::detail;

TEST_SUITE("Location Formatting") {
    TEST_CASE("Basic location formatting") {
        const char* file = "/home/user/project/src/main.cpp";
        int line = 42;
        
        SUBCASE("Default format (square brackets)") {
            std::string result = format_location(file, line);
            CHECK(result == "[/home/user/project/src/main.cpp:42]");
        }
        
        SUBCASE("Format with separator") {
            std::string result = format_location_with_separator(file, line, " - ");
            CHECK(result == "[/home/user/project/src/main.cpp:42] - ");
        }
    }
    
    TEST_CASE("Filename extraction") {
        SUBCASE("Unix path") {
            const char* path = "/home/user/project/src/main.cpp";
            const char* filename = extract_filename(path);
            CHECK(std::string(filename) == "main.cpp");
        }
        
        SUBCASE("Windows path") {
            const char* path = "C:\\Users\\user\\project\\src\\main.cpp";
            const char* filename = extract_filename(path);
            CHECK(std::string(filename) == "main.cpp");
        }
        
        SUBCASE("No path separators") {
            const char* path = "main.cpp";
            const char* filename = extract_filename(path);
            CHECK(std::string(filename) == "main.cpp");
        }
        
        SUBCASE("Null path") {
            const char* filename = extract_filename(nullptr);
            CHECK(std::string(filename) == "<unknown>");
        }
    }
    
    TEST_CASE("source_location struct") {
        source_location loc("test.cpp", 100);
        
        SUBCASE("format() method") {
            CHECK(loc.format() == "[test.cpp:100]");
        }
        
        SUBCASE("format_with_separator() method") {
            CHECK(loc.format_with_separator(" | ") == "[test.cpp:100] | ");
        }
        
        SUBCASE("Stream output") {
            std::ostringstream oss;
            oss << loc;
            CHECK(oss.str() == "[test.cpp:100]");
        }
    }
    
    TEST_CASE("append_location function") {
        std::ostringstream oss;
        append_location(oss, "file.cc", 25);
        CHECK(oss.str() == "[file.cc:25]");
    }
}

// Test different format styles (these would need recompilation with different macros)
TEST_SUITE("Location Format Styles (compile-time)") {
    TEST_CASE("Documentation of available styles") {
        // This test just documents what the different styles look like
        const char* file = "test.cpp";
        int line = 42;
        
        INFO("Style 0: [test.cpp:42]     - Square brackets (default)");
        INFO("Style 1: test.cpp:42:       - Colon separator");
        INFO("Style 2: (test.cpp:42)      - Parentheses");
        INFO("Style 3: test.cpp(42):      - Function-style");
        INFO("Style 4: @test.cpp:42       - At symbol prefix");
        INFO("Style 5: test.cpp:42 -      - Dash suffix");
        
        // Current style
        std::string current = format_location(file, line);
        MESSAGE("Current format style: " << current);
    }
}

// Test path styles (these would need recompilation with different macros)
TEST_SUITE("Location Path Styles (compile-time)") {
    TEST_CASE("Documentation of path styles") {
        const char* full_path = "/home/user/project/src/test.cpp";
        
        INFO("Style 0: Full path (default) - /home/user/project/src/test.cpp");
        INFO("Style 1: Filename only       - test.cpp");
        INFO("Style 2: Relative to root    - src/test.cpp");
        
        // Current style
        std::string current = format_file_path(full_path);
        MESSAGE("Current path style result: " << current);
    }
}