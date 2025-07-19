//
// Example demonstrating the debug trap functionality
//

// Uncomment one of these to test different modes:
// #define FAILSAFE_TRAP_MODE 0  // Normal exception throwing (default)
// #define FAILSAFE_TRAP_MODE 1  // Trap to debugger, then throw
// #define FAILSAFE_TRAP_MODE 2  // Trap to debugger only (no throw)

#include <iostream>
#include <failsafe/exception.hh>

void example_function(int value) {
    // This will throw or trap based on FAILSAFE_TRAP_MODE
    THROW_UNLESS(value >= 0, std::invalid_argument, "Value must be non-negative, got: ", value);
    
    std::cout << "Value is valid: " << value << std::endl;
}

void demonstrate_trap_macros() {
    std::cout << "\n=== Demonstrating TRAP macros ===\n";
    
    // TRAP_IF example
    int x = 5;
    TRAP_IF(x > 10, "This won't trap because x is ", x);
    std::cout << "TRAP_IF didn't trigger\n";
    
    // TRAP_UNLESS example (like assert)
    TRAP_UNLESS(x > 0, "x must be positive!");
    std::cout << "TRAP_UNLESS passed\n";
    
    // Uncomment to see a trap in action:
    // TRAP("Manual trap with message: x = ", x);
}

int main() {
    std::cout << "Exception trap demo\n";
    std::cout << "Current FAILSAFE_TRAP_MODE: " << FAILSAFE_TRAP_MODE << "\n\n";
    
    try {
        std::cout << "Calling with valid value:\n";
        example_function(42);
        
        std::cout << "\nCalling with invalid value:\n";
        example_function(-1);  // This will throw/trap
        
    } catch (const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }
    
    demonstrate_trap_macros();
    
    std::cout << "\nDemo completed successfully!\n";
    return 0;
}

/*
To test different modes:

1. Normal mode (default):
   $ g++ -std=c++20 -I../include exception_trap_demo.cc -o demo
   $ ./demo

2. Trap then throw mode:
   $ g++ -std=c++20 -I../include -DFAILSAFE_TRAP_MODE=1 exception_trap_demo.cc -o demo
   $ gdb ./demo
   (gdb) run
   
3. Trap only mode:
   $ g++ -std=c++20 -I../include -DFAILSAFE_TRAP_MODE=2 exception_trap_demo.cc -o demo
   $ gdb ./demo
   (gdb) run

4. Or use the shorthand:
   $ g++ -std=c++20 -I../include -DFAILSAFE_TRAP_ON_THROW exception_trap_demo.cc -o demo
*/