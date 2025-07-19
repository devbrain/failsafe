//
// Test MSVC compatibility with traditional macros
//

// Force fallback mode by not defining builtin support
#define FAILSAFE_NO_BUILTIN_SOURCE_LOCATION 1

#include <failsafe/detail/location_format.hh>
#include <iostream>

void test_at_line(const failsafe::detail::source_location& loc = FAILSAFE_CURRENT_LOCATION()) {
    std::cout << "Called from: " << loc.format() << std::endl;
}

void test_macro_style() {
    // This is how MSVC users would typically use it
    failsafe::detail::source_location loc(__FILE__, __LINE__);
    std::cout << "Macro-based location: " << loc.format() << std::endl;
}

int main() {
    std::cout << "=== MSVC Compatibility Test ===" << std::endl;
    
    // Test explicit construction (always works)
    failsafe::detail::source_location loc1("msvc_test.cpp", 100);
    std::cout << "Explicit: " << loc1.format() << std::endl;
    
    // Test macro-based construction
    test_macro_style();
    
    // Test with FAILSAFE_CURRENT_LOCATION macro
    test_at_line();
    
    // Test default constructor
    #if !defined(FAILSAFE_HAS_STD_SOURCE_LOCATION) && !defined(FAILSAFE_HAS_BUILTIN_SOURCE_LOCATION)
        failsafe::detail::source_location loc_default;
        std::cout << "Default (no source info): " << loc_default.format() << std::endl;
    #endif
    
    return 0;
}