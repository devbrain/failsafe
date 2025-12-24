# =============================================================================
# Failsafe Tests
# =============================================================================

# Helper function to create a test executable with standard configuration
function(failsafe_add_test TEST_NAME)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})

    add_executable(${TEST_NAME} ${ARG_SOURCES})
    target_link_libraries(${TEST_NAME} PRIVATE failsafe doctest::doctest)
    neutrino_target_warnings(${TEST_NAME})
    neutrino_target_sanitizers(${TEST_NAME})

    # Disable warnings that conflict with intentional test patterns
    if(NEUTRINO_COMPILER_IS_GCC)
        target_compile_options(${TEST_NAME} PRIVATE
            -Wno-duplicated-branches  # Tests intentionally use identical branches to verify macro expansion
        )
    endif()

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Main test executable (tests that don't need isolation)
failsafe_add_test(failsafe_unittest
    SOURCES
        main.cc
        test_string_formatters.cc
        test_string_utils.cc
        test_location_format.cc
        test_container_edge_cases.cc
)

# Portability test
add_executable(test_portability test_portability.cc)
target_link_libraries(test_portability PRIVATE failsafe)
neutrino_target_warnings(test_portability)
neutrino_target_sanitizers(test_portability)
add_test(NAME test_portability COMMAND test_portability)

# Tests that explicitly define LOGGER_MIN_LEVEL need their own executables
# to avoid cross-TU issues on some compilers (notably clang-cl on Windows)
failsafe_add_test(test_compile_time_filtering
    SOURCES main.cc test_compile_time_filtering.cc
)

failsafe_add_test(test_lazy_logging
    SOURCES main.cc test_lazy_logging.cc
)

failsafe_add_test(test_logger
    SOURCES main.cc test_logger.cc
)

# Exception-related tests in separate executables to isolate potential issues
failsafe_add_test(test_exception
    SOURCES main.cc test_exception.cc
)

failsafe_add_test(test_exception_trap
    SOURCES main.cc test_exception_trap.cc
)

failsafe_add_test(test_exception_chaining
    SOURCES main.cc test_exception_chaining.cc
)

failsafe_add_test(test_enforce
    SOURCES main.cc test_enforce.cc
)

failsafe_add_test(test_enforce_chaining
    SOURCES main.cc test_enforce_chaining.cc
)
