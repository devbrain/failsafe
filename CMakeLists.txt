cmake_minimum_required(VERSION 3.20)
project(failsafe)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to build tests
option(BUILD_WITH_TESTS "Build unit tests" ON)

# Define warning flags for our project
set(FAILSAFE_WARNING_FLAGS)

add_library(failsafe INTERFACE)
target_include_directories(failsafe INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(BUILD_WITH_TESTS)
    # Include FetchContent module for downloading test dependencies
    include(FetchContent)
    set(FETCHCONTENT_QUIET off)
    
    # Only fetch doctest when building tests
    add_subdirectory(ext/doctest)
    add_subdirectory(test)
endif()

