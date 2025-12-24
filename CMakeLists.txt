cmake_minimum_required(VERSION 3.20)

project(failsafe
    VERSION 1.0.0
    DESCRIPTION "A header-only C++ error handling library"
    LANGUAGES CXX
)

# =============================================================================
# Neutrino CMake Integration
# =============================================================================

include(FetchContent)

if(NOT DEFINED NEUTRINO_CMAKE_DIR)
    FetchContent_Declare(neutrino_cmake
        GIT_REPOSITORY https://github.com/devbrain/neutrino-cmake.git
        GIT_TAG master
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(neutrino_cmake)
    set(NEUTRINO_CMAKE_DIR "${neutrino_cmake_SOURCE_DIR}/cmake")
    list(APPEND CMAKE_MODULE_PATH "${NEUTRINO_CMAKE_DIR}")
endif()

include(NeutrinoInit)

# =============================================================================
# Options
# =============================================================================

neutrino_define_options(failsafe)

option(NEUTRINO_FAILSAFE_BUILD_DOCS
    "Build documentation with Doxygen"
    OFF
)

# =============================================================================
# Dependencies
# =============================================================================

include(${NEUTRINO_CMAKE_DIR}/deps/termcolor.cmake)
include(${NEUTRINO_CMAKE_DIR}/deps/utf8cpp.cmake)

neutrino_fetch_termcolor()
neutrino_fetch_utf8cpp()

# =============================================================================
# Library Target
# =============================================================================

add_library(failsafe INTERFACE)
add_library(neutrino::failsafe ALIAS failsafe)

target_include_directories(failsafe INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(failsafe INTERFACE
    termcolor::termcolor
    utf8cpp::utf8cpp
)

target_compile_features(failsafe INTERFACE cxx_std_17)

# =============================================================================
# Tests
# =============================================================================

if(NEUTRINO_FAILSAFE_BUILD_TESTS)
    include(${NEUTRINO_CMAKE_DIR}/deps/doctest.cmake)
    neutrino_fetch_doctest()

    enable_testing()
    add_subdirectory(test)
endif()

# =============================================================================
# Examples
# =============================================================================

if(NEUTRINO_FAILSAFE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# =============================================================================
# Documentation
# =============================================================================

if(NEUTRINO_FAILSAFE_BUILD_DOCS)
    find_package(Doxygen QUIET COMPONENTS dot)
    if(DOXYGEN_FOUND)
        message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")
        if(DOXYGEN_DOT_FOUND)
            message(STATUS "Graphviz dot found: ${DOXYGEN_DOT_EXECUTABLE}")
            set(DOXYGEN_DOT_PATH ${DOXYGEN_DOT_EXECUTABLE})
        else()
            message(STATUS "Graphviz dot not found. Graphs will not be generated.")
            set(DOXYGEN_DOT_PATH "")
        endif()

        # Configure Doxyfile from template
        set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

        # Create docs directory
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/docs)
        file(MAKE_DIRECTORY ${DOXYGEN_OUT})

        # Find all header files for dependencies
        file(GLOB_RECURSE FAILSAFE_HEADERS
            ${CMAKE_CURRENT_SOURCE_DIR}/include/failsafe/*.hh
            ${CMAKE_CURRENT_SOURCE_DIR}/include/failsafe/*.h
        )

        # Add custom targets for documentation
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            DEPENDS ${FAILSAFE_HEADERS} ${DOXYFILE_OUT}
            VERBATIM
        )

        add_custom_target(docs-view
            COMMAND ${CMAKE_COMMAND} -E echo "Opening documentation in default browser..."
            COMMAND xdg-open ${DOXYGEN_OUT}/html/index.html || open ${DOXYGEN_OUT}/html/index.html || start ${DOXYGEN_OUT}/html/index.html
            DEPENDS docs
            COMMENT "Opening documentation in browser"
            VERBATIM
        )

        message(STATUS "Documentation will be built in: ${DOXYGEN_OUT}")
        message(STATUS "Use 'make docs' to build documentation")
        message(STATUS "Use 'make docs-view' to build and open documentation")
    else()
        message(WARNING "Doxygen not found. Documentation will not be built.")
    endif()
endif()

# =============================================================================
# Installation
# =============================================================================

if(NEUTRINO_FAILSAFE_INSTALL)
    include(NeutrinoInstall)

    neutrino_install_headers(failsafe)

    neutrino_install_library(failsafe
        NAMESPACE neutrino::
        COMPATIBILITY SameMajorVersion
        DEPENDENCIES
            "find_dependency(termcolor REQUIRED)"
            "find_dependency(utf8cpp REQUIRED)"
    )

    # Install documentation if built
    if(NEUTRINO_FAILSAFE_BUILD_DOCS AND DOXYGEN_FOUND)
        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs/html
            DESTINATION share/doc/failsafe
            OPTIONAL
        )
    endif()
endif()

# =============================================================================
# Summary
# =============================================================================

if(NEUTRINO_PROJECT_IS_TOP_LEVEL)
    neutrino_print_options(failsafe)
endif()
