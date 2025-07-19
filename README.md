# Failsafe

[![C++17](https://img.shields.io/badge/C%2B%2B-17/20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Header-only](https://img.shields.io/badge/header--only-yes-brightgreen.svg)](include/failsafe)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

Failsafe is a modern C++ header-only library providing robust error handling and logging capabilities with a focus on developer experience, performance, and flexibility.

## Features

- 🚀 **Zero-overhead abstractions** - Features can be completely compiled out
- 🔒 **Thread-safe logging** with compile-time level filtering
- 🛡️ **Policy-based enforcement** mechanism inspired by Andrei Alexandrescu
- 🎯 **Flexible exception system** with debug trap modes
- 📍 **Portable source location** handling (C++20, builtins, or macros)
- 🔌 **Multiple logger backends** (stderr, POCO, gRPC/Abseil)
- 📦 **Header-only** - No linking required
- 🔧 **C++17/20 compatible** - Works with both standards

## Quick Start

```cpp
#include <failsafe/logger.hh>
#include <failsafe/enforce.hh>
#include <failsafe/exception.hh>

int main() {
    // Logging
    LOG_INFO("App", "Starting application");
    LOG_ERROR_IF(error_occurred, "Network", "Connection failed:", error_code);
    
    // Enforcement
    auto ptr = ENFORCE(malloc(size))("Memory allocation failed");
    auto file = ENFORCE(fopen(path, "r"))("Cannot open file:", path);
    
    // Exceptions
    THROW_IF(ptr == nullptr, std::runtime_error, "Null pointer detected");
    
    return 0;
}
```

## Installation

Failsafe is header-only. Simply copy the `include/failsafe` directory to your project or use CMake:

### CMake Integration

```cmake
# Using FetchContent (recommended)
include(FetchContent)
FetchContent_Declare(
    failsafe
    GIT_REPOSITORY https://github.com/yourusername/failsafe.git
    GIT_TAG main
)
FetchContent_MakeAvailable(failsafe)

target_link_libraries(your_target PRIVATE failsafe::failsafe)
```

Or clone and use as a subdirectory:

```bash
git clone https://github.com/yourusername/failsafe.git
```

```cmake
add_subdirectory(failsafe)
target_link_libraries(your_target PRIVATE failsafe::failsafe)
```

## Core Components

### Logger

Thread-safe logging with multiple levels and compile-time filtering:

```cpp
// Basic logging
LOG_INFO("MyApp", "User logged in:", username);
LOG_ERROR("Database", "Query failed:", query, "error:", error_msg);

// Conditional logging
LOG_DEBUG_IF(verbose, "Parser", "Token:", token, "at position:", pos);

// Category-based logging
LOG_WARN("Network", "Retry attempt:", attempt, "of", max_retries);
```

Configure the logger:

```cpp
// Set minimum level
logger::set_min_level(LOGGER_LEVEL_INFO);

// Use custom backend
logger::set_backend(make_cerr_backend(true, true, true));  // timestamp, thread_id, colors

// Set category-specific levels
logger::set_category_level("Database", LOGGER_LEVEL_DEBUG);
```

### Enforce

Policy-based enforcement that returns the validated value:

```cpp
// Basic enforcement
auto ptr = ENFORCE(get_pointer());  // Throws on null

// With custom message
auto result = ENFORCE(api_call())("API call failed with code:", error_code);

// Comparison enforcement
auto index = ENFORCE_IN_RANGE(idx, 0, size-1)("Index out of bounds");
auto positive = ENFORCE_GT(value, 0)("Value must be positive");

// Custom exception type
auto data = ENFORCE_THROW(load_data(), std::invalid_argument);
```

### Exception

Enhanced exception throwing with automatic source location:

```cpp
// Basic throwing
THROW(std::runtime_error, "Operation failed:", operation, "code:", code);

// Conditional throwing
THROW_IF(fd < 0, std::system_error, "Cannot open file:", filename);
THROW_UNLESS(is_valid(), std::logic_error, "Invalid state");

// Debug helpers
TRAP_IF(critical_error, "Critical error in module:", module_name);
DEBUG_TRAP_RELEASE_THROW(std::runtime_error, "Should not reach here");
```

### String Utilities

Advanced string formatting with type-safe message building:

```cpp
using namespace failsafe::detail;

// Automatic formatting for various types
auto msg = build_message("Count:", 42, "Time:", 100ms, "Path:", fs::path("/tmp"));

// Special formatters
auto hex_msg = build_message("Address:", hex(0xDEADBEEF), "Flags:", bin(0b10101));
auto upper = build_message("Status:", uppercase("active"));

// Container formatting
std::vector<int> data = {1, 2, 3, 4, 5};
auto limited = build_message("Data:", container(data, 3));  // "[1, 2, 3, ...]"
```

## Configuration

### Compile-time Options

Define these before including failsafe headers:

```cpp
// Set minimum log level (completely removes lower levels)
#define LOGGER_MIN_LEVEL LOGGER_LEVEL_INFO

// Set default exception type
#define FAILSAFE_DEFAULT_EXCEPTION MyCustomException

// Configure debug trap behavior
#define FAILSAFE_TRAP_MODE 2  // 0: normal, 1: trap+throw, 2: trap only

// Configure source location format
#define FAILSAFE_LOCATION_FORMAT_STYLE 1  // Various styles available
#define FAILSAFE_LOCATION_PATH_STYLE 1    // 0: full, 1: filename, 2: relative

// Disable thread safety (for single-threaded apps)
#define LOGGER_THREAD_SAFE 0
```

### Runtime Configuration

```cpp
// Logger configuration
logger::set_min_level(LOGGER_LEVEL_DEBUG);
logger::set_backend(my_custom_backend);
logger::set_category_level("Critical", LOGGER_LEVEL_TRACE);

// Custom backend example
auto my_backend = [](int level, const char* category, 
                     const char* file, int line,
                     const std::string& message) {
    // Your implementation
};
```

## Logger Backends

### Cerr Backend (Built-in)

```cpp
// Full featured
auto backend = make_cerr_backend(
    true,   // show timestamp
    true,   // show thread ID  
    true    // use ANSI colors
);
logger::set_backend(backend);
```

### POCO Integration

```cpp
#include <failsafe/logger/backend/poco_backend.hh>

// Configure POCO logging first
Poco::AutoPtr<Poco::ConsoleChannel> console(new Poco::ConsoleChannel);
Poco::Logger::root().setChannel(console);

// Use POCO backend
logger::set_backend(make_poco_backend());
```

### gRPC/Abseil Integration

```cpp
#include <failsafe/logger/backend/grpc_logger.hh>

// Initialize
logger::grpc::init_grpc_logging();
logger::grpc::sync_grpc_verbosity();

// All gRPC logs now go through failsafe logger
```

## Advanced Usage

### Custom Predicates for Enforce

```cpp
// Define custom predicate
struct is_even {
    bool check(int value) const { return value % 2 == 0; }
    const char* description() const { return "Value must be even"; }
};

// Use it
auto value = enforce<int, is_even>(get_number(), {}, "number", __FILE__, __LINE__);
```

### Debug Mode Integration

```cpp
// Use debug-only enforcement
DEBUG_ENFORCE(internal_invariant());

// Different behavior for debug/release
#ifdef NDEBUG
    #define VERIFY(expr) (expr)
#else
    #define VERIFY(expr) ENFORCE(expr)
#endif
```

### Performance Considerations

- Log statements below `LOGGER_MIN_LEVEL` are completely removed at compile time
- Disabled features have zero runtime cost
- Thread safety can be disabled for single-threaded applications
- Header-only design allows full optimization

## Examples

See the [examples](examples/) directory for complete examples:

- [basic_usage.cpp](examples/basic_usage.cpp) - Getting started
- [custom_backend.cpp](examples/custom_backend.cpp) - Implementing a custom logger backend
- [error_handling.cpp](examples/error_handling.cpp) - Comprehensive error handling patterns
- [string_formatting.cpp](examples/string_formatting.cpp) - Advanced string formatting

## Testing

```bash
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

## Documentation

Generate documentation with Doxygen:

```bash
doxygen Doxyfile
# Open docs/html/index.html
```

## Requirements

- C++17 or C++20 compiler
- CMake 3.14+ (for building tests/examples)
- Optional: POCO C++ Libraries (for POCO backend)
- Optional: gRPC/Abseil (for gRPC backend)

## Compiler Support

- GCC 7+
- Clang 5+
- MSVC 2017+

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Enforcement mechanism inspired by Andrei Alexandrescu's "Enforcements" article
- Debug trap implementation based on psnip/debug-trap
- Modern C++ design patterns and best practices

## Related Projects

- [spdlog](https://github.com/gabime/spdlog) - Fast C++ logging library
- [fmt](https://github.com/fmtlib/fmt) - Modern formatting library
- [expected](https://github.com/TartanLlama/expected) - C++11/14/17 std::expected implementation