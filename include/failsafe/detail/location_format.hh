/**
 * @file location_format.hh
 * @brief Source location formatting utilities
 * 
 * @details
 * This header provides centralized source location formatting used throughout
 * the failsafe library. Features include:
 * - Portable source location capture (C++20, builtins, or macros)
 * - Configurable location format styles
 * - Configurable path display options
 * - Consistent formatting across logger, exceptions, and enforce
 * 
 * Configuration options:
 * - FAILSAFE_LOCATION_FORMAT_STYLE: Choose location format (0-5)
 * - FAILSAFE_LOCATION_PATH_STYLE: Choose path display (0-2)
 * - FAILSAFE_PROJECT_ROOT: Set project root for relative paths
 * - FAILSAFE_NO_BUILTIN_SOURCE_LOCATION: Disable builtin support
 */
#pragma once

#include <string>
#include <sstream>
#include <cstring>

// Check for C++20 source_location support
#if __has_include(<source_location>) && __cplusplus >= 202002L
    #include <source_location>
    #if defined(__cpp_lib_source_location) && __cpp_lib_source_location >= 201907L
        #define FAILSAFE_HAS_STD_SOURCE_LOCATION 1
    #endif
#endif

// Check for builtin source location support
#ifndef FAILSAFE_HAS_STD_SOURCE_LOCATION
    #ifndef FAILSAFE_NO_BUILTIN_SOURCE_LOCATION
        #ifdef __has_builtin
            #if __has_builtin(__builtin_FILE) && __has_builtin(__builtin_LINE)
                #define FAILSAFE_HAS_BUILTIN_SOURCE_LOCATION 1
            #endif
        #elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
            #define FAILSAFE_HAS_BUILTIN_SOURCE_LOCATION 1
        #endif
    #endif
#endif

/**
 * @defgroup LocationConfig Location Format Configuration
 * @{
 */

/**
 * @brief Location format style
 * 
 * Controls how file:line information is formatted:
 * - 0: [file:line] - Square brackets (default)
 * - 1: file:line: - Colon separator  
 * - 2: (file:line) - Parentheses
 * - 3: file(line): - Function-style
 * - 4: @file:line - At symbol prefix
 * - 5: file:line - - Dash suffix
 */
#ifndef FAILSAFE_LOCATION_FORMAT_STYLE
#define FAILSAFE_LOCATION_FORMAT_STYLE 0
#endif

/**
 * @brief Path display style
 * 
 * Controls how file paths are displayed:
 * - 0: Full path (default)
 * - 1: Filename only
 * - 2: Relative to project root (requires FAILSAFE_PROJECT_ROOT)
 */
#ifndef FAILSAFE_LOCATION_PATH_STYLE
#define FAILSAFE_LOCATION_PATH_STYLE 0
#endif

/** @} */ // end of LocationConfig group

/**
 * @namespace failsafe::detail
 * @brief Internal implementation details
 */
namespace failsafe::detail {

    /**
     * @brief Extract filename from full path
     * 
     * @param path Full file path
     * @return Pointer to filename portion or "<unknown>" if null
     */
    inline const char* extract_filename(const char* path) {
        if (!path) return "<unknown>";
        
        const char* last_slash = nullptr;
        const char* p = path;
        
        while (*p) {
            if (*p == '/' || *p == '\\') {
                last_slash = p;
            }
            ++p;
        }
        
        return last_slash ? last_slash + 1 : path;
    }
    
    /**
     * @brief Format file path based on configuration
     * 
     * Applies FAILSAFE_LOCATION_PATH_STYLE to format the path.
     * 
     * @param file Raw file path
     * @return Formatted file path string
     */
    inline std::string format_file_path(const char* file) {
        if (!file) return "<unknown>";
        
        #if FAILSAFE_LOCATION_PATH_STYLE == 0
            // Full path
            return file;
        #elif FAILSAFE_LOCATION_PATH_STYLE == 1
            // Filename only
            return extract_filename(file);
        #elif FAILSAFE_LOCATION_PATH_STYLE == 2
            // Relative to project root
            #ifdef FAILSAFE_PROJECT_ROOT
                const char* root = FAILSAFE_PROJECT_ROOT;
                const char* relative = std::strstr(file, root);
                if (relative && relative == file) {
                    // Skip the root path and any following slash
                    file += std::strlen(root);
                    while (*file == '/' || *file == '\\') ++file;
                }
            #endif
            return file;
        #else
            return file;
        #endif
    }
    
    /**
     * @brief Format complete source location
     * 
     * Combines file and line using FAILSAFE_LOCATION_FORMAT_STYLE.
     * 
     * @param file Source file path
     * @param line Source line number
     * @return Formatted location string
     */
    inline std::string format_location(const char* file, int line) {
        std::ostringstream oss;
        std::string formatted_file = format_file_path(file);
        
        #if FAILSAFE_LOCATION_FORMAT_STYLE == 0
            // [file:line]
            oss << "[" << formatted_file << ":" << line << "]";
        #elif FAILSAFE_LOCATION_FORMAT_STYLE == 1
            // file:line:
            oss << formatted_file << ":" << line << ":";
        #elif FAILSAFE_LOCATION_FORMAT_STYLE == 2
            // (file:line)
            oss << "(" << formatted_file << ":" << line << ")";
        #elif FAILSAFE_LOCATION_FORMAT_STYLE == 3
            // file(line):
            oss << formatted_file << "(" << line << "):";
        #elif FAILSAFE_LOCATION_FORMAT_STYLE == 4
            // @file:line
            oss << "@" << formatted_file << ":" << line;
        #elif FAILSAFE_LOCATION_FORMAT_STYLE == 5
            // file:line -
            oss << formatted_file << ":" << line << " -";
        #else
            // Default to style 0
            oss << "[" << formatted_file << ":" << line << "]";
        #endif
        
        return oss.str();
    }
    
    /**
     * @brief Append formatted location to output stream
     * 
     * @param os Output stream
     * @param file Source file path
     * @param line Source line number
     */
    inline void append_location(std::ostream& os, const char* file, int line) {
        os << format_location(file, line);
    }
    
    /**
     * @brief Format location with custom separator
     * 
     * @param file Source file path
     * @param line Source line number
     * @param separator String to append after location
     * @return Formatted location with separator
     */
    inline std::string format_location_with_separator(const char* file, int line, 
                                                      const char* separator = " ") {
        return format_location(file, line) + separator;
    }
    
    /**
     * @brief Portable source location structure
     * 
     * Provides a unified interface for source location information across
     * different compiler capabilities (C++20, builtins, or traditional macros).
     */
    struct source_location {
        const char* file;
        int line;
        
        #if defined(FAILSAFE_HAS_STD_SOURCE_LOCATION)
        // C++20 std::source_location
        source_location(const std::source_location& loc = std::source_location::current())
            : file(loc.file_name()), line(static_cast<int>(loc.line())) {}
        
        source_location(const char* f, int l) 
            : file(f), line(l) {}
            
        #elif defined(FAILSAFE_HAS_BUILTIN_SOURCE_LOCATION)
        // GCC/Clang builtins
        source_location(const char* f = __builtin_FILE(), int l = __builtin_LINE()) 
            : file(f), line(l) {}
            
        #else
        // Fallback for MSVC and older compilers - requires macros
        source_location(const char* f, int l) 
            : file(f), line(l) {}
            
        // Default constructor uses dummy values
        source_location() 
            : file("<unknown>"), line(0) {}
        #endif
        
        /**
         * @brief Format this location
         * @return Formatted location string
         */
        std::string format() const {
            return format_location(file, line);
        }
        
        /**
         * @brief Format this location with separator
         * @param sep Separator to append
         * @return Formatted location with separator
         */
        std::string format_with_separator(const char* sep = " ") const {
            return format_location_with_separator(file, line, sep);
        }
        
        /**
         * @brief Stream output operator
         * @param os Output stream
         * @param loc Source location
         * @return Reference to stream
         */
        friend std::ostream& operator<<(std::ostream& os, const source_location& loc) {
            append_location(os, loc.file, loc.line);
            return os;
        }
    };
    
    /**
     * @brief Type alias for source_location
     */
    using location = source_location;
    
    /**
     * @brief Macro for portable source location capture
     * 
     * Uses the best available method based on compiler support:
     * - C++20 std::source_location if available
     * - Compiler builtins if available
     * - Traditional __FILE__/__LINE__ macros as fallback
     */
    #if defined(FAILSAFE_HAS_STD_SOURCE_LOCATION)
        #define FAILSAFE_CURRENT_LOCATION() ::failsafe::detail::source_location()
    #elif defined(FAILSAFE_HAS_BUILTIN_SOURCE_LOCATION)
        #define FAILSAFE_CURRENT_LOCATION() ::failsafe::detail::source_location()
    #else
        #define FAILSAFE_CURRENT_LOCATION() ::failsafe::detail::source_location(__FILE__, __LINE__)
    #endif

} // namespace failsafe::detail
