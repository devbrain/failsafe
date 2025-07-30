cmake_minimum_required (VERSION 3.14)
project (utf8cpp 
         VERSION 4.0.6
         LANGUAGES CXX
         DESCRIPTION "C++ portable library for working with utf-8 encoding")

add_library(${PROJECT_NAME} INTERFACE)

include(GNUInstallDirs)

target_include_directories(utf8cpp INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>"
    $<INSTALL_INTERFACE:include/utf8cpp>
)

# Skip all installation - handled by parent
set(CMAKE_SKIP_INSTALL_RULES ON)