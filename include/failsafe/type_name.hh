/**
 * @file type_name.hh
 * @brief Compile-time type names and hashes for diagnostics (C++17).
 *
 * Adapted from Adam Badura's Apache-2.0 type_name library, as used by BSW:
 * https://github.com/adambadura/type_name
 * See LICENSE for the Apache License, Version 2.0.
 *
 * Failsafe changes: namespaced API, consolidated compiler implementations,
 * array-safe parsing, stream insertion, and compact CRC32 hashing.
 */
#pragma once

#include <cstdint>
#include <ostream>
#include <string_view>

namespace failsafe {

    /**
     * @brief The compiler's spelling of T, available as a constexpr string_view.
     *
     * Preserves cv/ref qualifiers. No RTTI, allocation, or object is required.
     * The view refers to static storage, but is not necessarily null-terminated.
     * Names vary by compiler and standard library; use them for diagnostics,
     * not as persistent or portable type identifiers.
     */
    template<typename T>
    struct type_name {
    private:
        static constexpr auto get() noexcept {
            // clang-cl also defines _MSC_VER: use Clang's signature format first.
#if defined(__clang__)
            constexpr std::string_view signature{__PRETTY_FUNCTION__};
            constexpr std::string_view prefix{"[T = "};
            constexpr std::string_view suffix{"]"};
#elif defined(__GNUC__)
            // A deduced return type avoids GCC's extra string_view alias binding.
            constexpr std::string_view signature{__PRETTY_FUNCTION__};
            constexpr std::string_view prefix{"[with T = "};
            constexpr std::string_view suffix{"]"};
#elif defined(_MSC_VER)
            constexpr std::string_view signature{__FUNCSIG__};
            constexpr std::string_view prefix{"type_name<"};
            constexpr std::string_view suffix{">::get("};
#else
#error "failsafe::type_name requires GCC, Clang, or MSVC"
#endif
            constexpr auto marker = signature.find(prefix);
            static_assert(marker != std::string_view::npos,
                          "Unrecognized compiler type-name prefix");
            constexpr auto begin = marker + prefix.size();
            // Array types and nested templates can contain the same delimiter.
            constexpr auto end = signature.rfind(suffix);
            static_assert(end != std::string_view::npos && end > begin,
                          "Unrecognized compiler type-name suffix");
            return signature.substr(begin, end - begin);
        }

    public:
        using value_type = std::string_view;
        static constexpr value_type value{get()};

        constexpr operator value_type() const noexcept {
            return value;
        }

        constexpr value_type operator()() const noexcept {
            return value;
        }

        friend std::ostream& operator<<(std::ostream& stream, type_name) {
            return stream << value;
        }
    };

    template<typename T>
    inline constexpr std::string_view type_name_v = type_name<T>::value;

    namespace detail {
        // Same CRC32 as BSW's type_hash_v, without a per-evaluation lookup table.
        constexpr std::uint32_t type_name_crc32(std::string_view name) noexcept {
            std::uint32_t crc = 0xffffffffU;
            for (const char character : name) {
                crc ^= static_cast<unsigned char>(character);
                for (int bit = 0; bit < 8; ++bit) {
                    crc = (crc & 1U) != 0U ? (crc >> 1U) ^ 0xedb88320U : crc >> 1U;
                }
            }
            return crc ^ 0xffffffffU;
        }
    }

    /**
     * @brief CRC32 of the compiler-specific type name, evaluated at compile time.
     * Hashes can collide and can change with the compiler; not a unique type ID.
     */
    template<typename T>
    inline constexpr std::uint32_t type_hash_v = detail::type_name_crc32(type_name_v<T>);

} // namespace failsafe
