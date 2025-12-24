# =============================================================================
# Failsafe Examples
# =============================================================================

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

    # Examples use relaxed warnings - they're demonstration code
    if(NEUTRINO_COMPILER_IS_MSVC)
        target_compile_options(${example}_example PRIVATE /W3)
    else()
        target_compile_options(${example}_example PRIVATE
            -Wall
            -Wextra
            -Wno-unused-parameter
            -Wno-unused-variable
        )
    endif()
endforeach()

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

message(STATUS "Examples will be built in: ${CMAKE_BINARY_DIR}/examples")
