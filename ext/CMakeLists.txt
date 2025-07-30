# This file should not be used directly. 
# The parent CMakeLists.txt handles conditional inclusion of dependencies.
# If this file is included, it means all dependencies should be built.

include(FetchContent)
set(FETCHCONTENT_QUIET off)

# Only add subdirectories if they exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/termcolor/CMakeLists.txt)
    add_subdirectory(termcolor)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utfcpp/CMakeLists.txt)
    add_subdirectory(utfcpp)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/doctest/CMakeLists.txt)
    add_subdirectory(doctest)
endif()








