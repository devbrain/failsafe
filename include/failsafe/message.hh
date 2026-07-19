/**
 * @file message.hh
 * @brief Public message builder: concatenate arbitrary arguments into a string.
 *
 * @details
 * failsafe::build_message() is the space-joining, auto-stringifying string builder
 * that powers ENFORCE / THROW_* / LOG_*. It is a first-class, public API so callers
 * can form messages to *store or return* (e.g. a diagnostic's text) without reaching
 * into failsafe::detail.
 *
 * @code
 * // "resource 3 out of bounds: offset 4096"
 * auto msg = failsafe::build_message("resource", i, "out of bounds: offset", off);
 * @endcode
 *
 * Arguments are separated by single spaces; numbers and other streamable types are
 * formatted via failsafe::detail::append_to_stream (which custom formatters can extend).
 *
 * The canonical definition lives in namespace failsafe (in detail/string_utils.hh,
 * alongside the append_to_stream overloads it is built on); this header is the
 * discoverable public entry point. The legacy failsafe::detail::build_message spelling
 * remains available as an alias.
 */
#pragma once

#include <failsafe/detail/string_utils.hh>
