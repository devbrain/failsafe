cmake_minimum_required(VERSION 3.20)
project(failsafe
    VERSION 1.0.0
    DESCRIPTION "A header-only C++ error handling library"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(FAILSAFE_BUILD_WITH_TESTS "Build unit tests" ON)
option(FAILSAFE_BUILD_EXAMPLES "Build example programs" OFF)
option(FAILSAFE_BUILD_DOCUMENTATION "Build documentation with Doxygen" OFF)

# Define warning flags for our project
set(FAILSAFE_WARNING_FLAGS)

add_library(failsafe INTERFACE)
target_include_directories(failsafe INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Specify required C++ features instead of just the standard
target_compile_features(failsafe INTERFACE
    cxx_std_17
    cxx_constexpr
    cxx_noexcept
    cxx_nullptr
    cxx_auto_type
    cxx_decltype
    cxx_static_assert
    cxx_variadic_templates
    cxx_rvalue_references
    cxx_override
    cxx_final
    cxx_deleted_functions
    cxx_defaulted_functions
)

# Installation rules
include(GNUInstallDirs)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hh"
)

install(TARGETS failsafe
    EXPORT failsafeTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(FAILSAFE_BUILD_WITH_TESTS)
    # Try to find installed doctest first
    find_package(doctest 2.4 QUIET)
    
    if(doctest_FOUND)
        message(STATUS "Found installed doctest: ${doctest_VERSION}")
    else()
        message(STATUS "doctest not found, will fetch from GitHub")
        # Include FetchContent module for downloading test dependencies
        include(FetchContent)
        set(FETCHCONTENT_QUIET off)
        
        # Only fetch doctest when building tests
        add_subdirectory(ext/doctest)
    endif()
    
    add_subdirectory(test)
endif()

# Build examples if requested
if(FAILSAFE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Documentation building
if(FAILSAFE_BUILD_DOCUMENTATION)
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
        
        # Set project version for documentation
        if(NOT PROJECT_VERSION)
            set(PROJECT_VERSION "1.0")
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
        
        # Add a custom target to run Doxygen (built with ALL)
        add_custom_target(documentation ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            DEPENDS ${FAILSAFE_HEADERS} ${DOXYFILE_OUT}
            VERBATIM
        )
        
        # Add a target to just build docs (not part of ALL)
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            DEPENDS ${FAILSAFE_HEADERS} ${DOXYFILE_OUT}
            VERBATIM
        )
        
        # Add a target to view documentation
        add_custom_target(docs-view
            COMMAND ${CMAKE_COMMAND} -E echo "Opening documentation in default browser..."
            COMMAND xdg-open ${DOXYGEN_OUT}/html/index.html || open ${DOXYGEN_OUT}/html/index.html || start ${DOXYGEN_OUT}/html/index.html
            DEPENDS documentation
            COMMENT "Opening documentation in browser"
            VERBATIM
        )
        
        # Install documentation if requested
        install(DIRECTORY ${DOXYGEN_OUT}/html
            DESTINATION share/doc/failsafe
            OPTIONAL
        )
        
        message(STATUS "Documentation will be built in: ${DOXYGEN_OUT}")
        message(STATUS "Use 'make documentation' to build docs as part of all targets")
        message(STATUS "Use 'make docs' to build only documentation")
        message(STATUS "Use 'make docs-view' to build and open documentation")
    else()
        message(WARNING "Doxygen not found. Documentation will not be built.")
        message(STATUS "Install Doxygen to enable documentation generation.")
        message(STATUS "  Ubuntu/Debian: sudo apt-get install doxygen graphviz")
        message(STATUS "  macOS: brew install doxygen graphviz")
        message(STATUS "  Windows: download from https://www.doxygen.nl/download.html")
    endif()
endif()

# Print build configuration summary
message(STATUS "")
message(STATUS "Failsafe build configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests: ${FAILSAFE_BUILD_WITH_TESTS}")
message(STATUS "  Build examples: ${FAILSAFE_BUILD_EXAMPLES}")
message(STATUS "  Build documentation: ${FAILSAFE_BUILD_DOCUMENTATION}")
message(STATUS "")

# CMake package configuration
include(CMakePackageConfigHelpers)

# Create package config file
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/failsafeConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/failsafeConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/failsafe
)

# Create version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/failsafeConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Install CMake files
install(EXPORT failsafeTargets
    FILE failsafeTargets.cmake
    NAMESPACE failsafe::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/failsafe
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/failsafeConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/failsafeConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/failsafe
)

