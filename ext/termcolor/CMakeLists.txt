include(FetchContent)
set(FETCHCONTENT_QUIET off)

# Allow FetchContent_Populate to be used
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

fetchcontent_declare(
        TERMCOLOR
        GIT_REPOSITORY "https://github.com/ikalnytskyi/termcolor.git"
        GIT_TAG v2.1.0
        GIT_PROGRESS TRUE
        UPDATE_COMMAND ""
        # Patch the CMakeLists.txt after downloading
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different
                      ${CMAKE_CURRENT_SOURCE_DIR}/termcolor-no-install.cmake
                      <SOURCE_DIR>/CMakeLists.txt
)

# Only populate, don't add subdirectory yet
FetchContent_GetProperties(TERMCOLOR)
if(NOT termcolor_POPULATED)
    FetchContent_Populate(TERMCOLOR)
    
    # Export the source directory to parent scope
    set(TERMCOLOR_SOURCE_DIR ${termcolor_SOURCE_DIR} PARENT_SCOPE)
    
    # Now add the subdirectory with our patched CMakeLists.txt
    add_subdirectory(${termcolor_SOURCE_DIR} ${termcolor_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

