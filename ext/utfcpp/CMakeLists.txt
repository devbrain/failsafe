include(FetchContent)
set(FETCHCONTENT_QUIET off)

# Allow FetchContent_Populate to be used
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

fetchcontent_declare(
        UTFCPP
        GIT_REPOSITORY "https://github.com/nemtrif/utfcpp.git"
        GIT_TAG v4.0.6
        GIT_PROGRESS TRUE
        UPDATE_COMMAND ""
        # Patch the CMakeLists.txt after downloading
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different
                      ${CMAKE_CURRENT_SOURCE_DIR}/utfcpp-no-install.cmake
                      <SOURCE_DIR>/CMakeLists.txt
)

# Only populate, don't add subdirectory yet
FetchContent_GetProperties(UTFCPP)
if(NOT utfcpp_POPULATED)
    FetchContent_Populate(UTFCPP)
    
    # Export the source directory to parent scope
    set(UTFCPP_SOURCE_DIR ${utfcpp_SOURCE_DIR} PARENT_SCOPE)
    
    # Now add the subdirectory with our patched CMakeLists.txt
    add_subdirectory(${utfcpp_SOURCE_DIR} ${utfcpp_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()