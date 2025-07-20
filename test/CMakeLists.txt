add_executable(unittest
    main.cc
        test_string_formatters.cc
        test_string_utils.cc
        test_logger.cc
        test_exception.cc
        test_exception_trap.cc
        test_enforce.cc
        test_location_format.cc
        test_lazy_logging.cc
        test_exception_chaining.cc
        test_enforce_chaining.cc
        test_compile_time_filtering.cc
)

# Add portability test as a separate executable
add_executable(test_portability test_portability.cc)
target_link_libraries(test_portability PRIVATE failsafe)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
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
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(FAILSAFE_WARNING_FLAGS
            /W4             # Highest warning level
            /permissive-    # Strict standard conformance
            /Zc:__cplusplus # Report correct __cplusplus value
    )
endif()

target_link_libraries(unittest PRIVATE failsafe thirdparty::doctest)
target_compile_options(unittest PRIVATE ${FAILSAFE_WARNING_FLAG})