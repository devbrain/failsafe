/**
 * @file expected.hh
 * @brief Unified expected type abstraction (std::expected or tl::expected)
 *
 * @details
 * Provides failsafe::expected, failsafe::unexpected, failsafe::make_unexpected,
 * and failsafe::make_unexpectedf. Resolves to std::expected (C++23) when available
 * or tl::expected as a portable fallback for C++17/20.
 */
#pragma once

#include <version>
#include <string>
#include <type_traits>
#include <utility>
#include <failsafe/detail/string_utils.hh>

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202211L
    #include <expected>
    namespace failsafe {
        /**
         * @brief Alias for std::expected (C++23)
         */
        template<typename T, typename E>
        using expected = std::expected<T, E>;

        /**
         * @brief Alias for std::unexpected (C++23)
         */
        template<typename E>
        using unexpected = std::unexpected<E>;

        using unexpect_t = std::unexpect_t;
        inline constexpr unexpect_t unexpect = std::unexpect;

        template<typename E>
        using bad_expected_access = std::bad_expected_access<E>;

        /**
         * @brief Wrap an error into an unexpected value.
         *
         * Taken by value so it supports lvalues, rvalues, and explicit template arguments.
         */
        template<typename E>
        constexpr auto make_unexpected(E e) {
            return std::unexpected<E>(std::move(e));
        }

        inline constexpr bool using_std_expected = true;
    }
#else
    #if defined(_MSC_VER)
        #pragma warning(push)
    #elif defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wsign-conversion"
    #elif defined(__GNUC__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wundef"
        #pragma GCC diagnostic ignored "-Wsign-conversion"
    #endif

    #include <tl/expected.hpp>

    #if defined(_MSC_VER)
        #pragma warning(pop)
    #elif defined(__clang__)
        #pragma clang diagnostic pop
    #elif defined(__GNUC__)
        #pragma GCC diagnostic pop
    #endif

    namespace failsafe {
        /**
         * @brief Alias for tl::expected (fallback for C++17/20)
         */
        template<typename T, typename E>
        using expected = tl::expected<T, E>;

        /**
         * @brief Alias for tl::unexpected (fallback for C++17/20)
         */
        template<typename E>
        using unexpected = tl::unexpected<E>;

        using unexpect_t = tl::unexpect_t;
        inline constexpr unexpect_t unexpect = tl::unexpect;

        template<typename E>
        using bad_expected_access = tl::bad_expected_access<E>;

        /**
         * @brief Wrap an error into an unexpected value.
         *
         * Taken by value so it supports lvalues, rvalues, and explicit template arguments.
         */
        template<typename E>
        constexpr auto make_unexpected(E e) {
            return tl::unexpected<E>(std::move(e));
        }

        inline constexpr bool using_std_expected = false;
    }
#endif

namespace failsafe {
    /**
     * @brief Standard error type for string error descriptions
     */
    using error_type = std::string;

    /**
     * @brief Common result type for operations that don't return a value
     */
    using result = expected<void, error_type>;

    /**
     * @brief Create an unexpected value with formatted message arguments
     *
     * Formats all arguments into a single string using failsafe's message builder.
     */
    template<typename... Args>
    constexpr auto make_unexpectedf(Args&&... args) {
        return make_unexpected(build_message(std::forward<Args>(args)...));
    }

    /**
     * @brief Name of the active expected implementation
     */
    inline constexpr const char* expected_implementation() noexcept {
        return using_std_expected ? "std::expected" : "tl::expected";
    }
} // namespace failsafe
