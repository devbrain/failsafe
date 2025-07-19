//
// Test to verify portable source location support
//

#include <failsafe/detail/location_format.hh>
#include <iostream>

void test_source_location() {
    // Test default constructor
    failsafe::detail::source_location loc1;
    std::cout << "Default location: " << loc1.format() << std::endl;
    
    // Test with explicit values
    failsafe::detail::source_location loc2("test.cpp", 42);
    std::cout << "Explicit location: " << loc2.format() << std::endl;
    
    // Test macro
    auto loc3 = FAILSAFE_CURRENT_LOCATION();
    std::cout << "Current location: " << loc3.format() << std::endl;
    
    // Report which method is being used
    #if defined(FAILSAFE_HAS_STD_SOURCE_LOCATION)
        std::cout << "Using: C++20 std::source_location" << std::endl;
    #elif defined(FAILSAFE_HAS_BUILTIN_SOURCE_LOCATION)
        std::cout << "Using: GCC/Clang __builtin_FILE()/__builtin_LINE()" << std::endl;
    #else
        std::cout << "Using: Traditional __FILE__/__LINE__ macros" << std::endl;
    #endif
}

int main() {
    std::cout << "=== Source Location Portability Test ===" << std::endl;
    std::cout << "Compiler: ";
    #if defined(__clang__)
        std::cout << "Clang " << __clang_major__ << "." << __clang_minor__ << std::endl;
    #elif defined(__GNUC__)
        std::cout << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << std::endl;
    #elif defined(_MSC_VER)
        std::cout << "MSVC " << _MSC_VER << std::endl;
    #else
        std::cout << "Unknown" << std::endl;
    #endif
    
    std::cout << "C++ Standard: " << __cplusplus << std::endl;
    
    test_source_location();
    
    return 0;
}