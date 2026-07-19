/**
 * @file message.hh
 * @brief Public message builder: concatenate arbitrary arguments into a string.
 *
 * @details
 * build_message() is the space-joining, auto-stringifying string builder that
 * powers ENFORCE / THROW_* / LOG_*. It is exposed here as a first-class,
 * public API so callers can form messages to *store or return* (e.g. a
 * diagnostic's text) without reaching into failsafe::detail.
 *
 * @code
 * // "resource 3 out of bounds: offset 4096"
 * auto msg = failsafe::build_message("resource", i, "out of bounds: offset", off);
 * @endcode
 *
 * Arguments are separated by single spaces; numbers and other streamable types
 * are formatted via failsafe::detail::append_to_stream (which custom formatters
 * can extend). The canonical definition lives in namespace failsafe; the former
 * failsafe::detail::build_message spelling is kept as an alias for compatibility.
 */
#pragma once

#include <sstream>
#include <string>
#include <utility>

#include <failsafe/detail/string_utils.hh>

namespace failsafe {

    /**
     * @brief Build a message string from variadic arguments.
     *
     * Concatenates all arguments into a single string, separated by spaces.
     * Uses failsafe::detail::append_to_stream for type-specific formatting.
     *
     * @tparam Args Variadic template parameter pack.
     * @param args Arguments to concatenate.
     * @return The built message string.
     */
    template<typename... Args>
    std::string build_message(Args&&... args) {
        if constexpr (sizeof...(args) == 0) {
            return "";
        } else {
            std::ostringstream oss;
            ((detail::append_to_stream(oss, std::forward<Args>(args)), oss << " "), ...);
            std::string output = oss.str();
            // Remove trailing space
            if (!output.empty() && output.back() == ' ') {
                output.pop_back();
            }
            return output;
        }
    }

} // namespace failsafe

namespace failsafe::detail {
    // Back-compat: existing call sites (and the ENFORCE/THROW_*/LOG_* macros) spell
    // this failsafe::detail::build_message. Alias it to the canonical public symbol.
    using failsafe::build_message;
} // namespace failsafe::detail
