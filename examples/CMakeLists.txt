# Examples CMakeLists.txt

# Common settings for all examples
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)

# Define example programs
set(EXAMPLE_PROGRAMS
    basic_usage
    custom_backend
    error_handling
    string_formatting
)

# Build each example
foreach(example ${EXAMPLE_PROGRAMS})
    add_executable(${example}_example ${example}.cpp)
    target_link_libraries(${example}_example PRIVATE failsafe)
    
    # Use same warning flags as tests
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${example}_example PRIVATE
            -Wall -Wextra -Wpedantic
            -Wno-unused-parameter  # Examples might have unused params
            -Wno-unused-variable   # Examples might have unused vars for demonstration
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${example}_example PRIVATE /W4)
    endif()
endforeach()

# Special requirements for certain examples
if(TARGET string_formatting_example)
    # String formatting example uses filesystem
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_libraries(string_formatting_example PRIVATE stdc++fs)
    endif()
endif()

# Create list of example targets
set(EXAMPLE_TARGETS)
foreach(example ${EXAMPLE_PROGRAMS})
    list(APPEND EXAMPLE_TARGETS ${example}_example)
endforeach()

# Add a convenience target to build all examples
add_custom_target(examples
    DEPENDS ${EXAMPLE_TARGETS}
    COMMENT "Building all examples"
)

# Print message about examples
message(STATUS "Examples will be built in: ${CMAKE_BINARY_DIR}/examples")
message(STATUS "Use 'make examples' to build all examples")
message(STATUS "Or build individual examples: make basic_usage_example")