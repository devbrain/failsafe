/**
 * @file string_utils.hh
 * @brief String manipulation and formatting utilities
 */
#pragma once

#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <filesystem>
#include <chrono>
#include <optional>
#include <variant>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <tuple>
#include <vector>
#include <array>
#include <list>
#include <deque>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

// Include utf8cpp for wstring conversion
// Suppress sign conversion warnings from utf8.h template instantiations
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4365) // signed/unsigned mismatch
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wswitch-default"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif

#include <utf8.h>

#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// C++20 feature detection
#if __cplusplus >= 202002L
    #include <concepts>
    #include <ranges>
    #define FAILSAFE_HAS_CONCEPTS 1
#else
    #define FAILSAFE_HAS_CONCEPTS 0
#endif

// Compatibility helpers for C++17
#if __cplusplus < 202002L
namespace std {
    template<typename T>
    using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
}
#endif

namespace failsafe::detail {

    /**
     * @brief Thread-safe wrapper for gmtime
     * @param time Pointer to time_t value
     * @param result Pointer to tm struct to store result
     * @return Pointer to result on success, nullptr on failure
     */
    inline std::tm* safe_gmtime(const std::time_t* time, std::tm* result) {
#ifdef _WIN32
        return gmtime_s(result, time) == 0 ? result : nullptr;
#else
        return gmtime_r(time, result);
#endif
    }

    /**
     * @brief Thread-safe wrapper for localtime
     * @param time Pointer to time_t value
     * @param result Pointer to tm struct to store result
     * @return Pointer to result on success, nullptr on failure
     */
    inline std::tm* safe_localtime(const std::time_t* time, std::tm* result) {
#ifdef _WIN32
        return localtime_s(result, time) == 0 ? result : nullptr;
#else
        return localtime_r(time, result);
#endif
    }

    /**
     * @brief Type traits for type-safe formatting (C++17/20 compatible)
     */
#if FAILSAFE_HAS_CONCEPTS
    // C++20: Use concepts
    template<typename T>
    concept integral_formattable = std::is_integral_v<std::remove_cvref_t<T>>;

    template<typename T>
    concept pointer_formattable = std::is_pointer_v<std::remove_cvref_t<T>>;

    template<typename T>
    concept numeric_formattable = integral_formattable<T> || pointer_formattable<T>;
#else
    // C++17: Use type traits
    template<typename T>
    inline constexpr bool integral_formattable_v = std::is_integral_v<std::remove_cvref_t<T>>;

    template<typename T>
    inline constexpr bool pointer_formattable_v = std::is_pointer_v<std::remove_cvref_t<T>>;

    template<typename T>
    inline constexpr bool numeric_formattable_v = integral_formattable_v<T> || pointer_formattable_v<T>;
#endif

    /**
     * @brief Format wrapper for uppercase output
     *
     * This formatter converts the output to uppercase. It works with any type
     * and applies the transformation to the string representation.
     *
     * @tparam T The type of value to format
     */
    template<typename T>
    struct uppercase_format {
        using value_type = T;
        T value;
    };

    /**
     * @brief Format wrapper for lowercase output
     *
     * This formatter converts the output to lowercase. It works with any type
     * and applies the transformation to the string representation.
     *
     * @tparam T The type of value to format
     */
    template<typename T>
    struct lowercase_format {
        using value_type = T;
        T value;
    };

    /**
     * @brief Format wrapper for hexadecimal output
     *
     * This formatter outputs numeric values in hexadecimal format.
     * Only works with integral types, pointers, and containers of such types.
     *
     * @tparam T The type of value to format (must be numeric)
     */
    template<typename T
#if FAILSAFE_HAS_CONCEPTS
        > requires numeric_formattable<T>
#else
        , typename = std::enable_if_t<numeric_formattable_v<T>>
        >
#endif
    struct hex_format {
        using value_type = T;
        T value;
        size_t width = 0; ///< Minimum width (0 for no padding)
        bool show_base = true; ///< Whether to show "0x" prefix
        bool uppercase = false; ///< Whether to use uppercase letters (A-F vs a-f)
    };

    /**
     * @brief Format wrapper for octal output
     *
     * This formatter outputs integral values in octal format.
     * Only works with integral types and containers of integral types.
     *
     * @tparam T The type of value to format (must be integral)
     */
    template<typename T
#if FAILSAFE_HAS_CONCEPTS
        > requires integral_formattable<T>
#else
        , typename = std::enable_if_t<integral_formattable_v<T>>
        >
#endif
    struct oct_format {
        using value_type = T;
        T value;
        size_t width = 0; ///< Minimum width (0 for no padding)
        bool show_base = true; ///< Whether to show "0" prefix
    };

    /**
     * @brief Format wrapper for binary output
     *
     * This formatter outputs integral values in binary format.
     * Only works with integral types and containers of integral types.
     *
     * @tparam T The type of value to format (must be integral)
     */
    template<typename T
#if FAILSAFE_HAS_CONCEPTS
        > requires integral_formattable<T>
#else
        , typename = std::enable_if_t<integral_formattable_v<T>>
        >
#endif
    struct bin_format {
        using value_type = T;
        T value;
        size_t width = 0; ///< Minimum width (0 for no padding)
        bool show_base = true; ///< Whether to show "0b" prefix
        size_t group_size = 4; ///< Group bits (e.g., 4 for "1010 1111")
    };

    /**
     * @brief Factory function to create uppercase formatter
     *
     * @tparam T Type of value to format
     * @param value The value to format
     * @return uppercase_format<T> wrapper
     *
     * @code
     * set_error("Error:", uppercase("failed"));  // Output: "Error: FAILED"
     * @endcode
     */
    template<typename T>
    inline auto uppercase(T&& value) {
        if constexpr (std::is_array_v <std::remove_reference_t <T>>) {
            // For string literals and arrays, convert to string
            return uppercase_format <std::string>{std::string(value)};
        } else {
            return uppercase_format <std::remove_cvref_t <T>>{std::forward <T>(value)};
        }
    }

    /**
     * @brief Factory function to create lowercase formatter
     *
     * @tparam T Type of value to format
     * @param value The value to format
     * @return lowercase_format<T> wrapper
     *
     * @code
     * set_error("Status:", lowercase("READY"));  // Output: "Status: ready"
     * @endcode
     */
    template<typename T>
    inline auto lowercase(T&& value) {
        if constexpr (std::is_array_v <std::remove_reference_t <T>>) {
            // For string literals and arrays, convert to string
            return lowercase_format <std::string>{std::string(value)};
        } else {
            return lowercase_format <std::remove_cvref_t <T>>{std::forward <T>(value)};
        }
    }

    /**
     * @brief Factory function to create hexadecimal formatter
     *
     * @tparam T Type of value to format (must be numeric)
     * @param value The value to format
     * @param width Minimum width (0 for no padding)
     * @param show_base Whether to show "0x" prefix
     * @param uppercase Whether to use uppercase letters
     * @return hex_format<T> wrapper
     *
     * @code
     * set_error("Address:", hex(0xDEADBEEF));          // Output: "Address: 0xdeadbeef"
     * set_error("Byte:", hex(255, 2, true, true));    // Output: "Byte: 0xFF"
     * set_error("Value:", hex(42, 4, false));         // Output: "Value: 002a"
     * @endcode
     */
    template<typename T
#if FAILSAFE_HAS_CONCEPTS
        > requires numeric_formattable<T>
#else
        , typename = std::enable_if_t<numeric_formattable_v<T>>
        >
#endif
    inline auto hex(T&& value, unsigned int width = 0, bool show_base = true, bool uppercase = false) {
        return hex_format<std::remove_cvref_t<T>>{std::forward<T>(value), width, show_base, uppercase};
    }

    /**
     * @brief Factory function to create octal formatter
     *
     * @tparam T Type of value to format (must be integral)
     * @param value The value to format
     * @param width Minimum width (0 for no padding)
     * @param show_base Whether to show "0" prefix
     * @return oct_format<T> wrapper
     *
     * @code
     * set_error("Permissions:", oct(0755));     // Output: "Permissions: 0755"
     * set_error("Value:", oct(64, 0, false));  // Output: "Value: 100"
     * @endcode
     */
    template<typename T
#if FAILSAFE_HAS_CONCEPTS
        > requires integral_formattable<T>
#else
        , typename = std::enable_if_t<integral_formattable_v<T>>
        >
#endif
    inline auto oct(T&& value, unsigned int width = 0, bool show_base = true) {
        return oct_format<std::remove_cvref_t<T>>{std::forward<T>(value), width, show_base};
    }

    /**
     * @brief Type traits for container detection (C++17/20 compatible)
     */
#if FAILSAFE_HAS_CONCEPTS
    // C++20: Use concepts
    template<typename T>
    concept has_begin_end = requires(T t) {
        { t.begin() } -> std::input_or_output_iterator;
        { t.end() } -> std::input_or_output_iterator;
    };

    template<typename T>
    concept has_size = requires(T t) {
        { t.size() } -> std::convertible_to<std::size_t>;
    };

    template<typename T>
    concept is_array_like = requires {
        typename T::value_type;
        requires std::is_array_v<T> ||
                 requires { typename std::tuple_size<T>::type; };
    };

    template<typename T>
    concept is_string_like = std::is_same_v<std::remove_cvref_t<T>, std::string> ||
                             std::is_same_v<std::remove_cvref_t<T>, std::string_view> ||
                             std::is_same_v<std::remove_cvref_t<T>, std::wstring> ||
                             std::is_convertible_v<T, const char*> ||
                             std::is_convertible_v<T, const wchar_t*>;
#else
    // C++17: Use SFINAE detection
    template<typename T, typename = void>
    struct has_begin_end : std::false_type {};
    
    template<typename T>
    struct has_begin_end<T, std::void_t<
        decltype(std::declval<T>().begin()),
        decltype(std::declval<T>().end())>
    > : std::true_type {};
    
    template<typename T>
    inline constexpr bool has_begin_end_v = has_begin_end<T>::value;
    
    template<typename T, typename = void>
    struct has_size : std::false_type {};
    
    template<typename T>
    struct has_size<T, std::void_t<
        decltype(std::declval<T>().size())>
    > : std::true_type {};
    
    template<typename T>
    inline constexpr bool has_size_v = has_size<T>::value;
    
    template<typename T, typename = void>
    struct has_value_type : std::false_type {};
    
    template<typename T>
    struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type {};
    
    template<typename T, typename = void>
    struct has_tuple_size : std::false_type {};
    
    template<typename T>
    struct has_tuple_size<T, std::void_t<
        decltype(std::tuple_size<T>::value)>
    > : std::true_type {};
    
    template<typename T>
    inline constexpr bool is_array_like_v = 
        has_value_type<T>::value && (std::is_array_v<T> || has_tuple_size<T>::value);
    
    template<typename T>
    inline constexpr bool is_string_like_v = 
        std::is_same_v<std::remove_cvref_t<T>, std::string> ||
        std::is_same_v<std::remove_cvref_t<T>, std::string_view> ||
        std::is_same_v<std::remove_cvref_t<T>, std::wstring> ||
        std::is_convertible_v<T, const char*> ||
        std::is_convertible_v<T, const wchar_t*>;
#endif

#if FAILSAFE_HAS_CONCEPTS
    template<typename T>
    concept container_like = has_begin_end<T> && !is_string_like<T> && !requires { std::tuple_size<T>::value; };

    template<typename T>
    concept map_like = container_like<T> && requires(T t) {
        typename T::key_type;
        typename T::mapped_type;
        { t.begin()->first };
        { t.begin()->second };
    };

    template<typename T>
    concept set_like = container_like<T> && !map_like<T> && requires {
        typename T::key_type;
    };
#else
    template<typename T>
    inline constexpr bool container_like_v = 
        has_begin_end_v<T> && !is_string_like_v<T> && !has_tuple_size<T>::value;
    
    template<typename T, typename = void>
    struct is_map_like : std::false_type {};
    
    template<typename T>
    struct is_map_like<T, std::void_t<
        typename T::key_type,
        typename T::mapped_type,
        decltype(std::declval<T>().begin()->first),
        decltype(std::declval<T>().begin()->second)>
    > : std::true_type {};
    
    template<typename T>
    inline constexpr bool map_like_v = container_like_v<T> && is_map_like<T>::value;
    
    template<typename T, typename = void>
    struct is_set_like : std::false_type {};
    
    template<typename T>
    struct is_set_like<T, std::void_t<typename T::key_type>> : std::true_type {};
    
    template<typename T>
    inline constexpr bool set_like_v = container_like_v<T> && !map_like_v<T> && is_set_like<T>::value;
    
    // Detection for std::optional
    template<typename T, typename = void>
    struct is_optional : std::false_type {};
    
    template<typename T>
    struct is_optional<T, std::void_t<
        decltype(std::declval<T>().has_value()),
        decltype(*std::declval<T>())>
    > : std::true_type {};
    
    template<typename T>
    inline constexpr bool is_optional_v = is_optional<T>::value;
    
    // Detection for std::variant
    template<typename T, typename = void>
    struct is_variant : std::false_type {};
    
    template<typename... Ts>
    struct is_variant<std::variant<Ts...>> : std::true_type {};
    
    template<typename T>
    inline constexpr bool is_variant_v = is_variant<T>::value;
    
    // Detection for std::pair
    template<typename T, typename = void>
    struct is_pair : std::false_type {};
    
    template<typename T1, typename T2>
    struct is_pair<std::pair<T1, T2>> : std::true_type {};
    
    template<typename T>
    inline constexpr bool is_pair_v = is_pair<T>::value;
#endif

    /**
     * @brief Format wrapper for container output with customization options
     *
     * This formatter outputs containers (vector, list, set, map, etc.) with
     * customizable formatting options like size limits, starting index, delimiters.
     *
     * @tparam T The container type
     */
    template<typename T
#if FAILSAFE_HAS_CONCEPTS
        > requires container_like<T>
#else
        , typename = std::enable_if_t<container_like_v<T>>
        >
#endif
    struct container_format {
        using value_type = T;
        T value;
        std::size_t max_items = std::numeric_limits <std::size_t>::max(); ///< Maximum items to show
        std::size_t start_index = 0; ///< Starting index (0-based)
        std::string_view prefix = "["; ///< Container prefix
        std::string_view suffix = "]"; ///< Container suffix
        std::string_view delimiter = ", "; ///< Item delimiter
        std::string_view ellipsis = "..."; ///< Ellipsis for truncated output
        bool show_indices = false; ///< Show indices for sequences
        bool multiline = false; ///< Use multiline format
        std::string_view indent = "  "; ///< Indentation for multiline
    };

    /**
     * @brief Factory function to create container formatter
     *
     * @tparam T Container type
     * @param value The container to format
     * @param max_items Maximum number of items to display (0 = all)
     * @return container_format<T> wrapper
     *
     * @code
     * std::vector<int> vec = {1, 2, 3, 4, 5};
     * set_error("Vector:", container(vec));           // Output: "Vector: [1, 2, 3, 4, 5]"
     * set_error("Limited:", container(vec, 3));       // Output: "Limited: [1, 2, 3, ...]"
     * @endcode
     */
    template<typename T
#if FAILSAFE_HAS_CONCEPTS
        > requires container_like<T>
#else
        , typename = std::enable_if_t<container_like_v<T>>
        >
#endif
    inline auto container(T&& value, std::size_t max_items = 0) {
        return container_format<std::remove_cvref_t<T>>{
            std::forward<T>(value),
            max_items == 0 ? std::numeric_limits <std::size_t>::max() : max_items
        };
    }

    /**
     * @brief Factory function to create container formatter with custom options
     *
     * @tparam T Container type
     * @param value The container to format
     * @param config Lambda to configure options
     * @return container_format<T> wrapper
     *
     * @code
     * std::vector<int> vec = {1, 2, 3, 4, 5};
     * set_error("Custom:", container(vec, [](auto& fmt) {
     *     fmt.max_items = 3;
     *     fmt.show_indices = true;
     *     fmt.prefix = "{";
     *     fmt.suffix = "}";
     * })); // Output: "Custom: {[0]: 1, [1]: 2, [2]: 3, ...}"
     * @endcode
     */
    template<typename T, typename ConfigFunc
#if FAILSAFE_HAS_CONCEPTS
        > requires container_like<T> && std::invocable<ConfigFunc, container_format<std::remove_cvref_t<T>>&>
#else
        , typename = std::enable_if_t<
            container_like_v<T> && 
            std::is_invocable_v<ConfigFunc, container_format<std::remove_cvref_t<T>>&>
        >>
#endif
    inline auto container(T&& value, ConfigFunc&& config) {
        container_format<std::remove_cvref_t<T>> fmt{std::forward<T>(value)};
        std::forward<ConfigFunc>(config)(fmt);
        return fmt;
    }

    /**
     * @brief Factory function to create binary formatter
     *
     * @tparam T Type of value to format (must be integral)
     * @param value The value to format
     * @param width Minimum width (0 for no padding)
     * @param show_base Whether to show "0b" prefix
     * @param group_size Group bits (0 for no grouping)
     * @return bin_format<T> wrapper
     *
     * @code
     * set_error("Flags:", bin(0b10101111));           // Output: "Flags: 0b10101111"
     * set_error("Byte:", bin(255, 8, true, 4));      // Output: "Byte: 0b1111 1111"
     * set_error("Nibble:", bin(0xA, 4, false));      // Output: "Nibble: 1010"
     * @endcode
     */
    template<typename T
#if FAILSAFE_HAS_CONCEPTS
        > requires integral_formattable<T>
#else
        , typename = std::enable_if_t<integral_formattable_v<T>>
        >
#endif
    inline auto bin(T&& value, unsigned int width = 0, bool show_base = true, unsigned int group_size = 0) {
        return bin_format<std::remove_cvref_t<T>>{std::forward<T>(value), width, show_base, group_size};
    }

    // Forward declaration of the main append_to_stream template
    template<typename T>
    void append_to_stream(std::ostringstream& oss, T&& value);

    /**
     * @brief Append hexadecimal formatted value to stream
     */
#if FAILSAFE_HAS_CONCEPTS
    template<typename T>
    void append_to_stream(std::ostringstream& oss, const hex_format<T>& fmt) {
#else
    template<typename T, typename Enable>
    void append_to_stream(std::ostringstream& oss, const hex_format<T, Enable>& fmt) {
#endif
        auto flags = oss.flags();

        // Handle base prefix
        if (fmt.show_base) {
#if FAILSAFE_HAS_CONCEPTS
            if constexpr (pointer_formattable<T>) {
#else
            if constexpr (pointer_formattable_v<T>) {
#endif
                if (fmt.value != nullptr) {
                    oss << "0x";
                }
            } else {
                // Don't show prefix for zero with no width
                if (fmt.value != 0 || fmt.width > 0) {
                    oss << "0x";
                }
            }
        }

        // Set hex formatting
        oss << std::hex;
        if (fmt.uppercase) {
            oss << std::uppercase;
        }

        // Set width and fill
        if (fmt.width > 0) {
            oss << std::setw(static_cast<int>(fmt.width)) << std::setfill('0');
        }

        // Output value
#if FAILSAFE_HAS_CONCEPTS
        if constexpr (pointer_formattable<T>) {
#else
        if constexpr (pointer_formattable_v<T>) {
#endif
            if (fmt.value == nullptr) {
                oss.flags(flags); // Restore before outputting
                oss << "nullptr";
                return;
            }
            oss << reinterpret_cast<std::uintptr_t>(fmt.value);
        } else {
            // For signed types, cast to unsigned to avoid negative hex
            if constexpr (std::is_signed_v <T>) {
                // Cast to int for proper display (avoid char interpretation)
                if constexpr (sizeof(T) == 1) {
                    oss << static_cast <unsigned int>(static_cast <std::make_unsigned_t <T>>(fmt.value));
                } else {
                    oss << static_cast <std::make_unsigned_t <T>>(fmt.value);
                }
            } else {
                // Cast to int for proper display (avoid char interpretation)
                if constexpr (sizeof(T) == 1) {
                    oss << static_cast <unsigned int>(fmt.value);
                } else {
                    oss << fmt.value;
                }
            }
        }

        oss.flags(flags); // Restore flags
    }

    /**
     * @brief Append octal formatted value to stream
     */
#if FAILSAFE_HAS_CONCEPTS
    template<typename T>
    void append_to_stream(std::ostringstream& oss, const oct_format<T>& fmt) {
#else
    template<typename T, typename Enable>
    void append_to_stream(std::ostringstream& oss, const oct_format<T, Enable>& fmt) {
#endif
        auto flags = oss.flags();

        // Set octal formatting
        oss << std::oct;

        // Set width and fill
        if (fmt.width > 0 && fmt.show_base && fmt.value != 0) {
            // When showing base, the total width includes the '0' prefix
            // So we output "0" first, then the number with width-1
            oss << "0" << std::setw(static_cast<int>(fmt.width) - 1) << std::setfill('0');
        } else if (fmt.width > 0) {
            oss << std::setw(static_cast<int>(fmt.width)) << std::setfill('0');
        } else if (fmt.show_base && fmt.value != 0) {
            oss << "0";
        }

        // Output value
        if constexpr (std::is_signed_v <T>) {
            // Cast to int for proper display (avoid char interpretation)
            if constexpr (sizeof(T) == 1) {
                oss << static_cast <unsigned int>(static_cast <std::make_unsigned_t <T>>(fmt.value));
            } else {
                oss << static_cast <std::make_unsigned_t <T>>(fmt.value);
            }
        } else {
            // Cast to int for proper display (avoid char interpretation)
            if constexpr (sizeof(T) == 1) {
                oss << static_cast <unsigned int>(fmt.value);
            } else {
                oss << fmt.value;
            }
        }

        oss.flags(flags); // Restore flags
    }

    /**
     * @brief Append binary formatted value to stream
     */
#if FAILSAFE_HAS_CONCEPTS
    template<typename T>
    void append_to_stream(std::ostringstream& oss, const bin_format<T>& fmt) {
#else
    template<typename T, typename Enable>
    void append_to_stream(std::ostringstream& oss, const bin_format<T, Enable>& fmt) {
#endif
        // Handle base prefix
        if (fmt.show_base) {
            oss << "0b";
        }

        // Convert to unsigned for bit operations
        using UnsignedT = std::make_unsigned_t <T>;
        auto uvalue = static_cast <UnsignedT>(fmt.value);

        // Calculate number of bits
        constexpr int total_bits = sizeof(T) * 8;
        int bits_to_show = fmt.width > 0 ? static_cast<int>(fmt.width) : total_bits;

        // Find the highest set bit if width is 0
        if (fmt.width == 0 && uvalue != 0) {
            bits_to_show = 0;
            auto temp = uvalue;
            while (temp) {
                bits_to_show++;
                temp >>= 1;
            }
        } else if (fmt.width == 0 && uvalue == 0) {
            bits_to_show = 1; // Show at least one bit for zero
        }

        // Output bits
        std::string bit_string;
        size_t bit_count = 0;
        for (int i = bits_to_show - 1; i >= 0; --i) {
            if (bit_count > 0 && fmt.group_size > 0 && bit_count % fmt.group_size == 0) {
                bit_string += ' ';
            }
            bit_string += ((uvalue >> i) & 1) ? '1' : '0';
            bit_count++;
        }

        oss << bit_string;
    }

    // Add overloads for non-const lvalue references
#if FAILSAFE_HAS_CONCEPTS
    template<typename T>
    void append_to_stream(std::ostringstream& oss, hex_format<T>& fmt) {
        append_to_stream(oss, static_cast<const hex_format<T>&>(fmt));
    }
#else
    template<typename T, typename Enable>
    void append_to_stream(std::ostringstream& oss, hex_format<T, Enable>& fmt) {
        append_to_stream(oss, static_cast<const hex_format<T, Enable>&>(fmt));
    }
#endif

    // Add overloads for rvalue references
#if FAILSAFE_HAS_CONCEPTS
    template<typename T>
    void append_to_stream(std::ostringstream& oss, hex_format<T>&& fmt) {
        append_to_stream(oss, static_cast<const hex_format<T>&>(fmt));
    }
#else
    template<typename T, typename Enable>
    void append_to_stream(std::ostringstream& oss, hex_format<T, Enable>&& fmt) {
        append_to_stream(oss, static_cast<const hex_format<T, Enable>&>(fmt));
    }
#endif

#if FAILSAFE_HAS_CONCEPTS
    template<typename T>
    void append_to_stream(std::ostringstream& oss, oct_format<T>& fmt) {
        append_to_stream(oss, static_cast<const oct_format<T>&>(fmt));
    }
#else
    template<typename T, typename Enable>
    void append_to_stream(std::ostringstream& oss, oct_format<T, Enable>& fmt) {
        append_to_stream(oss, static_cast<const oct_format<T, Enable>&>(fmt));
    }
#endif

#if FAILSAFE_HAS_CONCEPTS
    template<typename T>
    void append_to_stream(std::ostringstream& oss, oct_format<T>&& fmt) {
        append_to_stream(oss, static_cast<const oct_format<T>&>(fmt));
    }
#else
    template<typename T, typename Enable>
    void append_to_stream(std::ostringstream& oss, oct_format<T, Enable>&& fmt) {
        append_to_stream(oss, static_cast<const oct_format<T, Enable>&>(fmt));
    }
#endif

#if FAILSAFE_HAS_CONCEPTS
    template<typename T>
    void append_to_stream(std::ostringstream& oss, bin_format<T>& fmt) {
        append_to_stream(oss, static_cast<const bin_format<T>&>(fmt));
    }
#else
    template<typename T, typename Enable>
    void append_to_stream(std::ostringstream& oss, bin_format<T, Enable>& fmt) {
        append_to_stream(oss, static_cast<const bin_format<T, Enable>&>(fmt));
    }
#endif

#if FAILSAFE_HAS_CONCEPTS
    template<typename T>
    void append_to_stream(std::ostringstream& oss, bin_format<T>&& fmt) {
        append_to_stream(oss, static_cast<const bin_format<T>&>(fmt));
    }
#else
    template<typename T, typename Enable>
    void append_to_stream(std::ostringstream& oss, bin_format<T, Enable>&& fmt) {
        append_to_stream(oss, static_cast<const bin_format<T, Enable>&>(fmt));
    }
#endif

    template<typename T>
    void append_to_stream(std::ostringstream& oss, uppercase_format <T>& fmt) {
        append_to_stream(oss, static_cast <const uppercase_format <T>&>(fmt));
    }

    template<typename T>
    void append_to_stream(std::ostringstream& oss, uppercase_format <T>&& fmt) {
        append_to_stream(oss, static_cast <const uppercase_format <T>&>(fmt));
    }

    template<typename T>
    void append_to_stream(std::ostringstream& oss, lowercase_format <T>& fmt) {
        append_to_stream(oss, static_cast <const lowercase_format <T>&>(fmt));
    }

    template<typename T>
    void append_to_stream(std::ostringstream& oss, lowercase_format <T>&& fmt) {
        append_to_stream(oss, static_cast <const lowercase_format <T>&>(fmt));
    }

    /**
     * @brief Helper to get container size if available
     */
    template<typename Container>
    inline std::size_t get_container_size(const Container& c) {
#if FAILSAFE_HAS_CONCEPTS
        if constexpr (has_size<Container>) {
            return c.size();
        } else {
            // For containers without size() like forward_list, count elements
            return std::distance(c.begin(), c.end());
        }
#else
        if constexpr (has_size_v<Container>) {
            return c.size();
        } else {
            // For containers without size() like forward_list, count elements
            return std::distance(c.begin(), c.end());
        }
#endif
    }

    /**
     * @brief Append container formatted value to stream
     */
#if FAILSAFE_HAS_CONCEPTS
    template<typename T>
    void append_to_stream(std::ostringstream& oss, const container_format<T>& fmt) {
#else
    template<typename T, typename Enable>
    void append_to_stream(std::ostringstream& oss, const container_format<T, Enable>& fmt) {
#endif
        const auto& container = fmt.value;
        auto size = get_container_size(container);

        // Calculate actual range to display
        auto start_it = container.begin();
        if (fmt.start_index < size) {
            std::advance(start_it, fmt.start_index);
        } else {
            // Start index beyond container size
            oss << fmt.prefix << fmt.suffix;
            return;
        }

        auto items_to_show = std::min(fmt.max_items, size - fmt.start_index);
        bool truncated = (fmt.start_index + items_to_show) < size;

        oss << fmt.prefix;

        if (fmt.multiline && items_to_show > 0) {
            oss << "\n";
        }

        std::size_t index = fmt.start_index;
        auto it = start_it;

        for (std::size_t i = 0; i < items_to_show && it != container.end(); ++i, ++it, ++index) {
            if (i > 0) {
                oss << fmt.delimiter;
                if (fmt.multiline) {
                    oss << "\n";
                }
            }

            if (fmt.multiline) {
                oss << fmt.indent;
            }

            if (fmt.show_indices) {
                oss << "[" << index << "]: ";
            }

            // Handle map-like containers specially
#if FAILSAFE_HAS_CONCEPTS
            if constexpr (map_like<T>) {
#else
            if constexpr (map_like_v<T>) {
#endif
                append_to_stream(oss, it->first);
                oss << ": ";
                append_to_stream(oss, it->second);
            } else {
                append_to_stream(oss, *it);
            }
        }

        if (truncated) {
            if (items_to_show > 0) {
                oss << fmt.delimiter;
                if (fmt.multiline) {
                    oss << "\n" << fmt.indent;
                }
            }
            oss << fmt.ellipsis;
        }

        if (fmt.multiline && (items_to_show > 0 || truncated)) {
            oss << "\n";
        }

        oss << fmt.suffix;
    }

    // Add overloads for non-const lvalue and rvalue references
#if FAILSAFE_HAS_CONCEPTS
    template<typename T>
    void append_to_stream(std::ostringstream& oss, container_format<T>& fmt) {
        append_to_stream(oss, static_cast<const container_format<T>&>(fmt));
    }

    template<typename T>
    void append_to_stream(std::ostringstream& oss, container_format<T>&& fmt) {
        append_to_stream(oss, static_cast<const container_format<T>&>(fmt));
    }
#else
    template<typename T, typename Enable>
    void append_to_stream(std::ostringstream& oss, container_format<T, Enable>& fmt) {
        append_to_stream(oss, static_cast<const container_format<T, Enable>&>(fmt));
    }

    template<typename T, typename Enable>
    void append_to_stream(std::ostringstream& oss, container_format<T, Enable>&& fmt) {
        append_to_stream(oss, static_cast<const container_format<T, Enable>&>(fmt));
    }
#endif

    /**
     * @brief Append uppercase formatted value to stream
     */
    template<typename T>
    void append_to_stream(std::ostringstream& oss, const uppercase_format <T>& fmt) {
        // First, get the string representation
        std::ostringstream temp;
        append_to_stream(temp, fmt.value);
        std::string str = temp.str();

        // Convert to uppercase
        std::transform(str.begin(), str.end(), str.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        oss << str;
    }

    /**
     * @brief Append lowercase formatted value to stream
     */
    template<typename T>
    void append_to_stream(std::ostringstream& oss, const lowercase_format <T>& fmt) {
        // First, get the string representation
        std::ostringstream temp;
        append_to_stream(temp, fmt.value);
        std::string str = temp.str();

        // Convert to lowercase
        std::transform(str.begin(), str.end(), str.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        oss << str;
    }

    /**
     * @brief Generic append_to_stream implementation
     *
     * This is the definition of the generic template declared above.
     * It must come after all the formatter specializations.
     */
    template<typename T>
    void append_to_stream(std::ostringstream& oss, T&& value) {
        using std::operator<<; // Enable ADL
        using DecayT = std::decay_t <T>;

        // Handle bool specially
        if constexpr (std::is_same_v <DecayT, bool>) {
            oss << (value ? "true" : "false");
        }
        // Handle nullptr (but not arrays/string literals)
        else if constexpr (std::is_pointer_v <DecayT> && !std::is_array_v <std::remove_reference_t <T>>) {
            if (value == nullptr) {
                oss << "nullptr";
            } else {
                oss << value;
            }
        }
        // Handle std::filesystem::path
        else if constexpr (std::is_same_v <DecayT, std::filesystem::path>) {
            oss << value.string();
        }
        // Handle std::chrono::duration types
#if FAILSAFE_HAS_CONCEPTS
        else if constexpr (requires { typename DecayT::rep; typename DecayT::period; value.count(); }) {
#else
        else if constexpr (std::is_same_v<DecayT, std::chrono::nanoseconds> ||
                          std::is_same_v<DecayT, std::chrono::microseconds> ||
                          std::is_same_v<DecayT, std::chrono::milliseconds> ||
                          std::is_same_v<DecayT, std::chrono::seconds> ||
                          std::is_same_v<DecayT, std::chrono::minutes> ||
                          std::is_same_v<DecayT, std::chrono::hours>) {
#endif
            if constexpr (std::is_same_v <DecayT, std::chrono::nanoseconds>) {
                oss << value.count() << "ns";
            } else if constexpr (std::is_same_v <DecayT, std::chrono::microseconds>) {
                oss << value.count() << "us";
            } else if constexpr (std::is_same_v <DecayT, std::chrono::milliseconds>) {
                oss << value.count() << "ms";
            } else if constexpr (std::is_same_v <DecayT, std::chrono::seconds>) {
                oss << value.count() << "s";
            } else if constexpr (std::is_same_v <DecayT, std::chrono::minutes>) {
                oss << value.count() << "min";
            } else if constexpr (std::is_same_v <DecayT, std::chrono::hours>) {
                oss << value.count() << "h";
            } else {
                // Generic duration handling
                oss << value.count() << " ticks";
            }
        }
        // Handle std::chrono::time_point
#if FAILSAFE_HAS_CONCEPTS
        else if constexpr (requires { value.time_since_epoch(); }) {
#else
        else if constexpr (std::is_same_v<DecayT, std::chrono::system_clock::time_point> ||
                          std::is_same_v<DecayT, std::chrono::steady_clock::time_point> ||
                          std::is_same_v<DecayT, std::chrono::high_resolution_clock::time_point>) {
#endif
            // Convert to system_clock time_point if possible
            if constexpr (std::is_convertible_v <DecayT, std::chrono::system_clock::time_point>) {
                auto tp = std::chrono::system_clock::time_point(value);
                auto time_t = std::chrono::system_clock::to_time_t(tp);

                // Get milliseconds part
                auto ms = std::chrono::duration_cast <std::chrono::milliseconds>(
                              tp.time_since_epoch()) % 1000;

                // Format as ISO 8601
                std::tm tm{};
                safe_gmtime(&time_t, &tm);
                oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
                oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
            } else {
                // For other clocks, just output the duration since epoch
                auto duration = value.time_since_epoch();
                append_to_stream(oss, duration);
                oss << " since epoch";
            }
        }
        // Handle std::optional
#if FAILSAFE_HAS_CONCEPTS
        else if constexpr (requires { value.has_value(); *value; }) {
#else
        else if constexpr (is_optional_v<DecayT>) {
#endif
            if (value.has_value()) {
                append_to_stream(oss, *value);
            } else {
                oss << "nullopt";
            }
        }
        // Handle std::variant
#if FAILSAFE_HAS_CONCEPTS
        else if constexpr (requires { std::visit([](auto&&) {}, value); }) {
#else
        else if constexpr (is_variant_v<DecayT>) {
#endif
            std::visit([&oss](auto&& arg) {
                using ArgT = std::decay_t <decltype(arg)>;
                if constexpr (std::is_same_v <ArgT, std::monostate>) {
                    oss << "monostate";
                } else {
                    append_to_stream(oss, std::forward <decltype(arg)>(arg));
                }
            }, value);
        }
        // Handle std::wstring before containers to avoid treating it as a container
        else if constexpr (std::is_same_v<DecayT, std::wstring>) {
            std::string utf8_string;
            // Check the size of wchar_t to determine encoding
            if constexpr (sizeof(wchar_t) == 2) {
                // Windows: UTF-16
                utf8::utf16to8(value.begin(), value.end(), std::back_inserter(utf8_string));
            } else if constexpr (sizeof(wchar_t) == 4) {
                // Linux/Unix: UTF-32
                utf8::utf32to8(value.begin(), value.end(), std::back_inserter(utf8_string));
            }
            oss << utf8_string;
        }
        // Handle containers (vector, list, set, map, etc.) - must come before tuple check
        // because std::array has tuple_size but should be treated as a container
#if FAILSAFE_HAS_CONCEPTS
        else if constexpr (container_like<DecayT> ||
                           (has_begin_end<DecayT> && requires { typename DecayT::value_type; } &&
                            !is_string_like<DecayT>)) {
            // Default formatting for containers
            if constexpr (map_like<DecayT>) {
#else
        else if constexpr (container_like_v<DecayT> ||
                           (has_begin_end_v<DecayT> && has_value_type<DecayT>::value &&
                            !is_string_like_v<DecayT>)) {
            // Default formatting for containers
            if constexpr (map_like_v<DecayT>) {
#endif
                // Maps use braces by default
                oss << "{";
                bool first = true;
                for (const auto& [key, val] : value) {
                    if (!first) oss << ", ";
                    first = false;
                    append_to_stream(oss, key);
                    oss << ": ";
                    append_to_stream(oss, val);
                }
                oss << "}";
#if FAILSAFE_HAS_CONCEPTS
            } else if constexpr (set_like<DecayT>) {
#else
            } else if constexpr (set_like_v<DecayT>) {
#endif
                // Sets use braces by default
                oss << "{";
                bool first = true;
                for (const auto& item : value) {
                    if (!first) oss << ", ";
                    first = false;
                    append_to_stream(oss, item);
                }
                oss << "}";
            } else {
                // Sequences use brackets by default
                oss << "[";
                bool first = true;
                for (const auto& item : value) {
                    if (!first) oss << ", ";
                    first = false;
                    append_to_stream(oss, item);
                }
                oss << "]";
            }
        }
        // Handle std::pair
#if FAILSAFE_HAS_CONCEPTS
        else if constexpr (requires { value.first; value.second; }) {
#else
        else if constexpr (is_pair_v<DecayT>) {
#endif
            oss << "(";
            append_to_stream(oss, value.first);
            oss << ", ";
            append_to_stream(oss, value.second);
            oss << ")";
        }
        // Handle std::tuple
#if FAILSAFE_HAS_CONCEPTS
        else if constexpr (requires { std::tuple_size<DecayT>::value; }) {
#else
        else if constexpr (has_tuple_size<DecayT>::value && !std::is_array_v<DecayT>) {
#endif
            oss << "(";
            std::apply([&oss, first = true](auto&&... args) mutable {
                ((first ? (first = false, void()) : (oss << ", ", void()),
                  append_to_stream(oss, std::forward <decltype(args)>(args))), ...);
            }, value);
            oss << ")";
        }
        // Handle std::monostate directly
        else if constexpr (std::is_same_v <DecayT, std::monostate>) {
            oss << "monostate";
        }
        // Default case - use operator<<
        else {
            oss << std::forward <T>(value);
        }
    }

    /**
     * @brief Specialization for std::wstring to handle UTF-16/UTF-32 to UTF-8 conversion
     */
    inline void append_to_stream(std::ostringstream& oss, const std::wstring& value) {
        std::string utf8_string;
        
        // Check the size of wchar_t to determine encoding
        if constexpr (sizeof(wchar_t) == 2) {
            // Windows: UTF-16
            utf8::utf16to8(value.begin(), value.end(), std::back_inserter(utf8_string));
        } else if constexpr (sizeof(wchar_t) == 4) {
            // Linux/Unix: UTF-32
            // Convert to u32string first to avoid sign-conversion warning
            // (wchar_t is signed on Linux, but char32_t is unsigned)
            std::u32string u32_temp(value.begin(), value.end());
            utf8::utf32to8(u32_temp.begin(), u32_temp.end(), std::back_inserter(utf8_string));
        }
        
        oss << utf8_string;
    }

    /**
     * @brief Overload for non-const std::wstring
     */
    inline void append_to_stream(std::ostringstream& oss, std::wstring& value) {
        append_to_stream(oss, static_cast<const std::wstring&>(value));
    }

    /**
     * @brief Overload for rvalue std::wstring
     */
    inline void append_to_stream(std::ostringstream& oss, std::wstring&& value) {
        append_to_stream(oss, static_cast<const std::wstring&>(value));
    }

    /**
     * @brief Specialization for const wchar_t* (wide string literals)
     */
    inline void append_to_stream(std::ostringstream& oss, const wchar_t* value) {
        if (value) {
            append_to_stream(oss, std::wstring(value));
        } else {
            oss << "nullptr";
        }
    }

    /**
     * @brief Overload for non-const wchar_t*
     */
    inline void append_to_stream(std::ostringstream& oss, wchar_t* value) {
        append_to_stream(oss, static_cast<const wchar_t*>(value));
    }

    /**
     * @brief Specialization for std::wstring_view
     */
    inline void append_to_stream(std::ostringstream& oss, const std::wstring_view& value) {
        append_to_stream(oss, std::wstring(value));
    }

    /**
     * @brief Overload for non-const std::wstring_view
     */
    inline void append_to_stream(std::ostringstream& oss, std::wstring_view& value) {
        append_to_stream(oss, static_cast<const std::wstring_view&>(value));
    }

    /**
     * @brief Overload for rvalue std::wstring_view
     */
    inline void append_to_stream(std::ostringstream& oss, std::wstring_view&& value) {
        append_to_stream(oss, static_cast<const std::wstring_view&>(value));
    }

    /**
     * @brief Build a message string from variadic arguments
     *
     * Concatenates all arguments into a single string, separated by spaces.
     * Uses append_to_stream for special formatting of various types.
     *
     * @tparam Args Variadic template parameter pack
     * @param args Arguments to concatenate
     * @return The built message string
     */
    template<typename... Args>
    std::string build_message(Args&&... args) {
        if constexpr (sizeof...(args) == 0) {
            return "";
        } else {
            std::ostringstream oss;
            ((append_to_stream(oss, std::forward <Args>(args)), oss << " "), ...);
            std::string output = oss.str();
            // Remove trailing space
            if (!output.empty() && output.back() == ' ') {
                output.pop_back();
            }
            return output;
        }
    }

    /**
     * @page custom_formatters Creating Custom Formatters
     *
     * To create your own formatter for use with sdlpp's string building functions,
     * follow these steps:
     *
     * ## 1. Define a Formatter Struct
     *
     * Create a struct that holds your value and any formatting options:
     *
     * @code
     * template<typename T>
     * struct my_format {
     *     T value;
     *     // Add any formatting options
     *     int precision = 2;
     *     bool show_sign = true;
     * };
     * @endcode
     *
     * ## 2. Add Type Constraints (Optional)
     *
     * Use concepts to restrict which types can use your formatter:
     *
     * @code
     * template<typename T>
     * concept my_formattable = std::is_floating_point_v<T> ||
     *                          std::is_integral_v<T>;
     *
     * template<typename T>
     * requires my_formattable<T>
     * struct my_format { ... };
     * @endcode
     *
     * ## 3. Create a Factory Function
     *
     * Provide a convenient way to create your formatter:
     *
     * @code
     * template<typename T>
     * requires my_formattable<T>
     * inline auto my_formatter(T&& value, int precision = 2) {
     *     return my_format<std::remove_cvref_t<T>>{
     *         std::forward<T>(value), precision
     *     };
     * }
     * @endcode
     *
     * ## 4. Implement append_to_stream Overload
     *
     * Add a specialization of append_to_stream for your formatter:
     *
     * @code
     * template<typename T>
     * void append_to_stream(std::ostringstream& oss, const my_format<T>& fmt) {
     *     auto flags = oss.flags();  // Save stream state
     *
     *     // Apply your formatting
     *     oss << std::fixed << std::setprecision(fmt.precision);
     *     if (fmt.show_sign && fmt.value >= 0) {
     *         oss << '+';
     *     }
     *     oss << fmt.value;
     *
     *     oss.flags(flags);  // Restore stream state
     * }
     * @endcode
     *
     * ## 5. Handle Complex Types (Optional)
     *
     * Support containers like optional and variant:
     *
     * @code
     * // Support for std::optional
     * template<typename T>
     * void append_to_stream(std::ostringstream& oss,
     *                      const my_format<std::optional<T>>& fmt) {
     *     if (fmt.value.has_value()) {
     *         append_to_stream(oss, my_format<T>{*fmt.value, fmt.precision});
     *     } else {
     *         oss << "none";
     *     }
     * }
     * @endcode
     *
     * ## Complete Example: Scientific Notation Formatter
     *
     * @code
     * namespace sdlpp::detail {
     *     // Type constraint
     *     template<typename T>
     *     concept scientific_formattable = std::is_floating_point_v<T>;
     *
     *     // Formatter struct
     *     template<typename T>
     *     requires scientific_formattable<T>
     *     struct sci_format {
     *         T value;
     *         int precision = 6;
     *         bool uppercase = false;
     *     };
     *
     *     // Factory function
     *     template<typename T>
     *     requires scientific_formattable<T>
     *     inline auto sci(T value, int precision = 6, bool uppercase = false) {
     *         return sci_format<T>{value, precision, uppercase};
     *     }
     *
     *     // Implementation
     *     template<typename T>
     *     void append_to_stream(std::ostringstream& oss, const sci_format<T>& fmt) {
     *         auto flags = oss.flags();
     *
     *         oss << std::scientific << std::setprecision(fmt.precision);
     *         if (fmt.uppercase) {
     *             oss << std::uppercase;
     *         }
     *         oss << fmt.value;
     *
     *         oss.flags(flags);
     *     }
     * }
     *
     * // Usage:
     * set_error("Value:", sci(3.14159e10, 3));  // Output: "Value: 3.142e+10"
     * @endcode
     *
     * ## Best Practices
     *
     * 1. **Always save and restore stream flags** to avoid side effects
     * 2. **Use concepts** to provide clear compile-time errors
     * 3. **Provide sensible defaults** for formatting options
     * 4. **Document your formatter** with examples
     * 5. **Consider composability** - can your formatter work with others?
     * 6. **Test edge cases** like zero, negative numbers, special values
     */
}
