/**
 * @file string_formatting.cpp
 * @brief Advanced string formatting examples
 * 
 * This example demonstrates the string formatting utilities provided
 * by Failsafe for building messages efficiently.
 */

#include <failsafe/detail/string_utils.hh>
#include <failsafe/logger.hh>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <array>
#include <list>
#include <chrono>
#include <thread>
#include <filesystem>
#include <optional>
#include <variant>

using namespace failsafe;
using namespace failsafe::detail;

// Custom type for demonstration
struct Point {
    double x, y;
    
    // Make it streamable
    friend std::ostream& operator<<(std::ostream& os, const Point& p) {
        return os << "(" << p.x << ", " << p.y << ")";
    }
};

// Example 1: Basic Type Formatting
void basic_formatting_examples() {
    std::cout << "\n=== Basic Type Formatting ===\n";
    
    // Integers
    std::cout << "Integer: " << build_message("Value:", 42) << "\n";
    std::cout << "Negative: " << build_message("Temperature:", -15, "°C") << "\n";
    
    // Floating point
    std::cout << "Float: " << build_message("Pi:", 3.14159) << "\n";
    std::cout << "Scientific: " << build_message("Avogadro:", 6.022e23) << "\n";
    
    // Strings
    std::cout << "String: " << build_message("Hello,", "World!") << "\n";
    std::string name = "Alice";
    std::cout << "Variable: " << build_message("User:", name) << "\n";
    
    // Characters
    std::cout << "Char: " << build_message("Grade:", 'A') << "\n";
    
    // Booleans
    std::cout << "Bool: " << build_message("Success:", true, "Failed:", false) << "\n";
    
    // Pointers
    int value = 42;
    std::cout << "Pointer: " << build_message("Address:", &value) << "\n";
    std::cout << "Null: " << build_message("Ptr:", nullptr) << "\n";
    
    // Custom types
    Point p{3.14, 2.71};
    std::cout << "Custom: " << build_message("Point:", p) << "\n";
}

// Example 2: Number Base Formatting
void number_base_formatting() {
    std::cout << "\n=== Number Base Formatting ===\n";
    
    uint32_t value = 0xDEADBEEF;
    
    // Hexadecimal
    std::cout << "Hex: " << build_message("Value:", hex(value)) << "\n";
    std::cout << "Hex (8-bit): " << build_message("Byte:", hex(uint8_t(0xFF))) << "\n";
    
    // Binary
    std::cout << "Binary: " << build_message("Flags:", bin(0b10101010)) << "\n";
    std::cout << "Binary (16-bit): " << build_message("Word:", bin(uint16_t(0b1111000011110000))) << "\n";
    
    // Octal
    std::cout << "Octal: " << build_message("Permissions:", oct(0755)) << "\n";
    
    // Mixed bases
    uint32_t color = 0x00FF00;
    std::cout << "Color: " << build_message(
        "RGB:", hex(color), 
        "=", "R:", (color >> 16) & 0xFF,
        "G:", (color >> 8) & 0xFF,
        "B:", color & 0xFF
    ) << "\n";
}

// Example 3: Container Formatting
void container_formatting() {
    std::cout << "\n=== Container Formatting ===\n";
    
    // Vector
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::cout << "Vector: " << build_message("Numbers:", container(numbers)) << "\n";
    std::cout << "Limited: " << build_message("First 5:", container(numbers, 5)) << "\n";
    
    // Array - need to convert to vector for C++17 compatibility
    std::array<std::string, 3> colors = {"red", "green", "blue"};
    std::vector<std::string> color_vec(colors.begin(), colors.end());
    std::cout << "Array: " << build_message("Colors:", container(color_vec)) << "\n";
    
    // Set
    std::set<int> unique = {3, 1, 4, 1, 5, 9, 2, 6};
    std::cout << "Set: " << build_message("Unique:", container(unique)) << "\n";
    
    // Map
    std::map<std::string, int> scores = {
        {"Alice", 95},
        {"Bob", 87},
        {"Charlie", 92}
    };
    std::cout << "Map: " << build_message("Scores:", container(scores, 2)) << "\n";
    
    // List
    std::list<double> values = {1.1, 2.2, 3.3, 4.4};
    std::cout << "List: " << build_message("Values:", container(values)) << "\n";
    
    // Nested containers
    std::vector<std::vector<int>> matrix = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };
    std::cout << "Matrix: " << build_message("2D:", container(matrix)) << "\n";
    
    // Empty container
    std::vector<int> empty;
    std::cout << "Empty: " << build_message("Items:", container(empty)) << "\n";
}

// Example 4: String Case Conversion
void case_conversion_examples() {
    std::cout << "\n=== String Case Conversion ===\n";
    
    std::string mixed = "Hello World! 123";
    
    std::cout << "Original: " << build_message(mixed) << "\n";
    std::cout << "Upper: " << build_message(uppercase(mixed)) << "\n";
    std::cout << "Lower: " << build_message(lowercase(mixed)) << "\n";
    
    // In combined messages
    std::string status = "warning";
    std::cout << "Status: " << build_message(
        "Level:", uppercase(status), 
        "- Please check logs"
    ) << "\n";
    
    // With string_view
    std::string_view sv = "String View Example";
    std::cout << "View Upper: " << build_message(uppercase(sv)) << "\n";
}

// Example 5: Chrono Types
void chrono_formatting() {
    std::cout << "\n=== Chrono Type Formatting ===\n";
    
    using namespace std::chrono;
    
    // Durations
    std::cout << "Milliseconds: " << build_message("Time:", 1500ms) << "\n";
    std::cout << "Seconds: " << build_message("Duration:", 45s) << "\n";
    std::cout << "Minutes: " << build_message("Elapsed:", 5min) << "\n";
    std::cout << "Hours: " << build_message("Total:", 2h) << "\n";
    
    // Mixed durations
    auto total_time = 1h + 30min + 45s;
    std::cout << "Total: " << build_message("Time:", total_time) << "\n";
    
    // Time points
    auto now = system_clock::now();
    std::cout << "Now: " << build_message("Timestamp:", now) << "\n";
    
    // Duration calculations
    auto start = steady_clock::now();
    std::this_thread::sleep_for(10ms);
    auto end = steady_clock::now();
    std::cout << "Operation took: " << build_message(end - start) << "\n";
}

// Example 6: Filesystem Paths
void filesystem_formatting() {
    std::cout << "\n=== Filesystem Path Formatting ===\n";
    
    namespace fs = std::filesystem;
    
    fs::path home = "/home/user";
    fs::path file = home / "documents" / "report.pdf";
    
    std::cout << "Path: " << build_message("File:", file) << "\n";
    std::cout << "Parent: " << build_message("Dir:", file.parent_path()) << "\n";
    std::cout << "Filename: " << build_message("Name:", file.filename()) << "\n";
    std::cout << "Extension: " << build_message("Ext:", file.extension()) << "\n";
}

// Example 7: Optional and Variant
void modern_cpp_types() {
    std::cout << "\n=== Modern C++ Types ===\n";
    
    // Optional
    std::optional<int> maybe_value = 42;
    std::optional<int> no_value;
    
    std::cout << "Has value: " << build_message("Optional:", maybe_value.value_or(-1)) << "\n";
    std::cout << "No value: " << build_message("Optional:", no_value.value_or(-1)) << "\n";
    
    // Variant
    std::variant<int, double, std::string> var = "Hello";
    std::cout << "Variant: " << build_message("Value:", 
        std::visit([](auto&& arg) -> std::string {
            std::stringstream ss;
            ss << arg;
            return ss.str();
        }, var)
    ) << "\n";
}

// Example 8: Complex Message Building
void complex_messages() {
    std::cout << "\n=== Complex Message Building ===\n";
    
    // Error report
    int error_code = 404;
    std::string url = "https://api.example.com/users/123";
    auto timestamp = std::chrono::system_clock::now();
    std::map<std::string, std::string> headers = {
        {"Content-Type", "application/json"},
        {"Authorization", "Bearer xyz..."}
    };
    
    auto error_msg = build_message(
        "HTTP Error:", error_code, "\n",
        "URL:", url, "\n",
        "Time:", timestamp, "\n",
        "Headers:", container(headers, 5)
    );
    
    std::cout << "Error Report:\n" << error_msg << "\n\n";
    
    // Performance metrics
    struct Metrics {
        size_t requests = 15234;
        double avg_latency = 45.7;
        double p99_latency = 125.3;
        size_t errors = 23;
    } metrics;
    
    auto perf_msg = build_message(
        "Performance Report:\n",
        "  Requests: ", metrics.requests, "\n",
        "  Avg Latency: ", metrics.avg_latency, "ms\n",
        "  P99 Latency: ", metrics.p99_latency, "ms\n",
        "  Error Rate: ", 
        double(metrics.errors) / metrics.requests * 100, "%"
    );
    
    std::cout << perf_msg << "\n";
}

// Example 9: Using with Logger
void logger_integration() {
    std::cout << "\n=== Logger Integration ===\n";
    
    // The logger uses the same formatting internally
    std::vector<int> data = {10, 20, 30, 40, 50};
    Point location{45.5, -122.6};
    
    LOG_INFO("Processing started",
             "Data points:", container(data, 3),
             "Location:", location);
    
    LOG_DEBUG("Memory address:", hex(&data),
              "Size:", data.size() * sizeof(int), "bytes");
    
    // Complex error with multiple formatters
    try {
        throw std::runtime_error("Connection failed");
    } catch (const std::exception& e) {
        LOG_ERROR("Operation failed",
                  "Error:", e.what(),
                  "Timestamp:", std::chrono::system_clock::now(),
                  "Retry count:", 3,
                  "Next retry in:", std::chrono::seconds(30));
    }
}

int main() {
    std::cout << "=== Advanced String Formatting Examples ===\n";
    
    basic_formatting_examples();
    number_base_formatting();
    container_formatting();
    case_conversion_examples();
    chrono_formatting();
    filesystem_formatting();
    modern_cpp_types();
    complex_messages();
    logger_integration();
    
    std::cout << "\n=== All examples completed ===\n";
    return 0;
}