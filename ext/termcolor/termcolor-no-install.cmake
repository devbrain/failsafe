cmake_minimum_required(VERSION 3.0)
project(termcolor)

#
# target
#

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_11)

#
# tests
#

option(TERMCOLOR_TESTS "Build termcolor tests." OFF)

if(TERMCOLOR_TESTS)
  add_executable(test_${PROJECT_NAME} test/test.cpp test/subtest.cpp)
  target_link_libraries(
    test_${PROJECT_NAME} ${PROJECT_NAME}::${PROJECT_NAME})
endif()

# Skip all installation - handled by parent
set(CMAKE_SKIP_INSTALL_RULES ON)