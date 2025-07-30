include(FetchContent)
set(FETCHCONTENT_QUIET off)

# Allow FetchContent_Populate to be used
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

set(DOCTEST_WITH_TESTS OFF CACHE INTERNAL "")
set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE INTERNAL "")
set(DOCTEST_NO_INSTALL ON CACHE INTERNAL "")

fetchcontent_declare(
        DOCTEST
        GIT_REPOSITORY "https://github.com/onqtam/doctest.git"
        GIT_TAG v2.4.12
        GIT_PROGRESS TRUE
        UPDATE_COMMAND ""
        # Patch the CMakeLists.txt after downloading to update CMake version
        PATCH_COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=<SOURCE_DIR> -P ${CMAKE_CURRENT_SOURCE_DIR}/patch_doctest.cmake
)

# Only populate, don't add subdirectory yet
FetchContent_GetProperties(DOCTEST)
if(NOT doctest_POPULATED)
    FetchContent_Populate(DOCTEST)
    
    # Export the source directory to parent scope
    set(DOCTEST_SOURCE_DIR ${doctest_SOURCE_DIR} PARENT_SCOPE)
    
    # Now add the subdirectory with our patched CMakeLists.txt
    add_subdirectory(${doctest_SOURCE_DIR} ${doctest_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

