# Main test executable (tests that don't need isolation)
add_executable(failsafe_unittest
    main.cc
    test_string_formatters.cc
    test_string_utils.cc
    test_location_format.cc
    test_container_edge_cases.cc
    strings_helper.hh
)

# Add portability test as a separate executable
add_executable(test_portability test_portability.cc)
target_link_libraries(test_portability PRIVATE failsafe)

# Tests that explicitly define LOGGER_MIN_LEVEL need their own executables
# to avoid cross-TU issues on some compilers (notably clang-cl on Windows)
add_executable(test_compile_time_filtering
    main.cc
    test_compile_time_filtering.cc
)
target_link_libraries(test_compile_time_filtering PRIVATE failsafe doctest::doctest)

add_executable(test_lazy_logging
    main.cc
    test_lazy_logging.cc
)
target_link_libraries(test_lazy_logging PRIVATE failsafe doctest::doctest)

add_executable(test_logger
    main.cc
    test_logger.cc
)
target_link_libraries(test_logger PRIVATE failsafe doctest::doctest)

# Exception-related tests in separate executables to isolate potential issues
add_executable(test_exception
    main.cc
    test_exception.cc
)
target_link_libraries(test_exception PRIVATE failsafe doctest::doctest)

add_executable(test_exception_trap
    main.cc
    test_exception_trap.cc
)
target_link_libraries(test_exception_trap PRIVATE failsafe doctest::doctest)

add_executable(test_exception_chaining
    main.cc
    test_exception_chaining.cc
)
target_link_libraries(test_exception_chaining PRIVATE failsafe doctest::doctest)

add_executable(test_enforce
    main.cc
    test_enforce.cc
)
target_link_libraries(test_enforce PRIVATE failsafe doctest::doctest)

add_executable(test_enforce_chaining
    main.cc
    test_enforce_chaining.cc
)
target_link_libraries(test_enforce_chaining PRIVATE failsafe doctest::doctest)

# Detect clang-cl (Clang with MSVC frontend on Windows)
set(FAILSAFE_IS_CLANG_CL FALSE)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(FAILSAFE_IS_CLANG_CL TRUE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT FAILSAFE_IS_CLANG_CL)
    set(FAILSAFE_WARNING_FLAGS
            -Wall           # All standard warnings
            -Wextra         # Extra warnings
            -Wpedantic      # Strict ISO C++ compliance
            -Wcast-align    # Warn about potential performance problem casts
            -Wcast-qual     # Warn about casts that remove qualifiers
            -Wconversion    # Warn about type conversions that may lose data
            -Wdouble-promotion  # Warn about float to double promotion
            -Wformat=2      # Additional format string warnings
            -Wnon-virtual-dtor  # Warn about non-virtual destructors
            -Wold-style-cast    # Warn about C-style casts
            -Woverloaded-virtual # Warn about overloaded virtual functions
            -Wshadow        # Warn about variable shadowing
            -Wsign-conversion   # Warn about sign conversions
            -Wundef         # Warn about undefined identifiers in #if
            -Wunused        # Warn about unused entities
            -Wzero-as-null-pointer-constant # Warn about using 0 as nullptr
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        list(APPEND FAILSAFE_WARNING_FLAGS
                -Wlogical-op    # Warn about logical operations being used where bitwise were probably wanted
                -Wuseless-cast  # Warn about useless casts
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR FAILSAFE_IS_CLANG_CL)
    set(FAILSAFE_WARNING_FLAGS
            /W4             # Highest warning level
            /permissive-    # Strict standard conformance
            /Zc:__cplusplus # Report correct __cplusplus value
    )
    if(FAILSAFE_IS_CLANG_CL)
        # Suppress noisy clang-cl warnings
        list(APPEND FAILSAFE_WARNING_FLAGS
                # Suppress C++ compatibility warnings (we require C++17)
                /clang:-Wno-c++98-compat
                /clang:-Wno-c++98-compat-pedantic
                /clang:-Wno-c++98-c++11-compat-binary-literal
                /clang:-Wno-pre-c++14-compat
                /clang:-Wno-pre-c++17-compat
                # Suppress overly strict warnings
                /clang:-Wno-unsafe-buffer-usage
                /clang:-Wno-exit-time-destructors
                /clang:-Wno-global-constructors
                /clang:-Wno-missing-prototypes
        )
    endif()
endif()

# Apply warning flags to all test executables
target_link_libraries(failsafe_unittest PRIVATE failsafe doctest::doctest)
target_compile_options(failsafe_unittest PRIVATE ${FAILSAFE_WARNING_FLAGS})
target_compile_options(test_compile_time_filtering PRIVATE ${FAILSAFE_WARNING_FLAGS})
target_compile_options(test_lazy_logging PRIVATE ${FAILSAFE_WARNING_FLAGS})
target_compile_options(test_logger PRIVATE ${FAILSAFE_WARNING_FLAGS})
target_compile_options(test_exception PRIVATE ${FAILSAFE_WARNING_FLAGS})
target_compile_options(test_exception_trap PRIVATE ${FAILSAFE_WARNING_FLAGS})
target_compile_options(test_exception_chaining PRIVATE ${FAILSAFE_WARNING_FLAGS})
target_compile_options(test_enforce PRIVATE ${FAILSAFE_WARNING_FLAGS})
target_compile_options(test_enforce_chaining PRIVATE ${FAILSAFE_WARNING_FLAGS})

# Register tests with CTest
include(CTest)
add_test(NAME failsafe_unittest COMMAND failsafe_unittest)
add_test(NAME test_portability COMMAND test_portability)
add_test(NAME test_compile_time_filtering COMMAND test_compile_time_filtering)
add_test(NAME test_lazy_logging COMMAND test_lazy_logging)
add_test(NAME test_logger COMMAND test_logger)
add_test(NAME test_exception COMMAND test_exception)
add_test(NAME test_exception_trap COMMAND test_exception_trap)
add_test(NAME test_exception_chaining COMMAND test_exception_chaining)
add_test(NAME test_enforce COMMAND test_enforce)
add_test(NAME test_enforce_chaining COMMAND test_enforce_chaining)
