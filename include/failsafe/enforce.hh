/**
 * @file enforce.hh
 * @brief Policy-based enforcement mechanism inspired by Andrei Alexandrescu
 * 
 * @details
 * This header provides a flexible enforcement system that:
 * - Validates conditions and throws exceptions on failure
 * - Returns the checked value for further use
 * - Supports custom predicates for validation logic
 * - Supports custom raisers for error handling
 * - Allows chaining with custom error messages
 * - Integrates with the failsafe exception system
 * 
 * Based on concepts from Andrei Alexandrescu's article:
 * "Enforcements" (C/C++ Users Journal, June 2003)
 * 
 * @example
 * @code
 * // Basic enforcement - throws on null
 * auto ptr = enforce(get_pointer());
 * 
 * // With custom message
 * auto file = enforce(fopen(filename, "r"))("Failed to open file:", filename);
 * 
 * // Custom predicates
 * auto positive = enforce<predicates::greater_than<int>>(value, {0});
 * auto in_bounds = enforce<predicates::in_range<int, int>>(index, {0, size-1});
 * 
 * // Custom exception types
 * auto valid = enforce<predicates::truth, raisers::raiser<std::invalid_argument>>(ptr);
 * 
 * // Convenience functions
 * auto result = enforce_not_null(ptr);
 * auto positive = enforce_positive(value);
 * @endcode
 */
#pragma once

#include <type_traits>
#include <utility>
#include <sstream>
#include <functional>

#include <failsafe/exception.hh>
#include <failsafe/detail/string_utils.hh>
#include <failsafe/message.hh>
#include <failsafe/detail/location_format.hh>

/**
 * @namespace failsafe::enforce
 * @brief Policy-based enforcement utilities
 */
namespace failsafe::enforce {

    // Forward declarations
    template<typename T, typename Predicate, typename Raiser>
    class enforcer;

    /**
     * @namespace failsafe::enforce::predicates
     * @brief Predicate policies for enforcement
     * 
     * Predicates determine how values are tested for validity.
     */
    /**
     * @brief Detection trait / concept for expected-like types
     *
     * Matches any type with has_value(), operator*(), and error() —
     * e.g. std::expected, tl::expected, boost::outcome, etc.
     */
#if FAILSAFE_HAS_CONCEPTS
    template<typename T>
    concept expected_like = requires(T t) {
        { t.has_value() } -> std::convertible_to<bool>;
        { *t };
        { t.error() };
    };
#else
    template<typename T, typename = void>
    inline constexpr bool expected_like_v = false;

    template<typename T>
    inline constexpr bool expected_like_v<T, std::void_t<
        decltype(std::declval<T>().has_value()),
        decltype(*std::declval<T>()),
        decltype(std::declval<T>().error())
    >> = true;
#endif

    namespace predicates {

        /**
         * @brief Default predicate - checks for truthiness
         * 
         * For pointers: checks non-null
         * For other types: converts to bool
         */
        struct truth {
            template<typename T>
            static bool check(const T& value) {
                if constexpr (std::is_pointer_v<T>) {
                    return value != nullptr;
                } else {
                    return static_cast<bool>(value);
                }
            }
            
            static const char* description() { return "Expression must be true"; }
        };
        
        /**
         * @brief Equality predicate
         * @tparam Expected Type of expected value
         */
        template<typename Expected>
        struct equal_to {
            Expected expected; ///< The expected value
            
            template<typename T>
            bool check(const T& value) const {
                return value == expected;
            }
            
            const char* description() const { return "Values must be equal"; }
        };
        
        /**
         * @brief Inequality predicate
         * @tparam Expected Type of value to compare against
         */
        template<typename Expected>
        struct not_equal_to {
            Expected expected; ///< The value that should not match
            
            template<typename T>
            bool check(const T& value) const {
                return value != expected;
            }
            
            const char* description() const { return "Values must not be equal"; }
        };
        
        /**
         * @brief Less-than predicate
         * @tparam Bound Type of upper bound
         */
        template<typename Bound>
        struct less_than {
            Bound bound; ///< Upper bound (exclusive)
            
            template<typename T>
            bool check(const T& value) const {
                return value < bound;
            }
            
            const char* description() const { return "Value must be less than bound"; }
        };
        
        /**
         * @brief Greater-than predicate
         * @tparam Bound Type of lower bound
         */
        template<typename Bound>
        struct greater_than {
            Bound bound; ///< Lower bound (exclusive)
            
            template<typename T>
            bool check(const T& value) const {
                return value > bound;
            }
            
            const char* description() const { return "Value must be greater than bound"; }
        };
        
        /**
         * @brief Range predicate (inclusive)
         * @tparam Lower Type of lower bound
         * @tparam Upper Type of upper bound
         */
        template<typename Lower, typename Upper>
        struct in_range {
            Lower lower; ///< Lower bound (inclusive)
            Upper upper; ///< Upper bound (inclusive)
            
            template<typename T>
            bool check(const T& value) const {
                return value >= lower && value <= upper;
            }
            
            const char* description() const { return "Value must be in range"; }
        };
        
    } // namespace predicates
    
    /**
     * @namespace failsafe::enforce::raisers
     * @brief Raiser policies for enforcement failures
     * 
     * Raisers determine how to handle enforcement failures.
     */
    namespace raisers {
        
        /**
         * @brief Default raiser - uses FAILSAFE_DEFAULT_EXCEPTION
         */
        struct default_raiser {
            template<typename... Args>
            #if FAILSAFE_TRAP_MODE == 2
            [[noreturn]]
            #endif
            static void raise(const char* file, int line, Args&&... args) {
                ::failsafe::exception::internal::throw_exception<FAILSAFE_DEFAULT_EXCEPTION>(
                    file, line, std::forward<Args>(args)...);
            }
        };
        
        /**
         * @brief Raiser for specific exception type
         * @tparam Exception The exception type to throw
         */
        template<typename Exception>
        struct exception_raiser {
            template<typename... Args>
            #if FAILSAFE_TRAP_MODE == 2
            [[noreturn]]
            #endif
            static void raise(const char* file, int line, Args&&... args) {
                ::failsafe::exception::internal::throw_exception<Exception>(
                    file, line, std::forward<Args>(args)...);
            }
        };
        
        /**
         * @brief Trap raiser - always traps to debugger
         */
        struct trap_raiser {
            template<typename... Args>
            [[noreturn]] static void raise(const char* file, int line, Args&&... args) {
                ::failsafe::exception::internal::print_exception_info(file, line,
                    ::failsafe::detail::build_message(std::forward<Args>(args)...));
                psnip_trap();
                std::terminate();
            }
        };
        
    } // namespace raisers
    
    /**
     * @brief Main enforcer class
     * 
     * Validates a value using a predicate and handles failures using a raiser.
     * Can be used with implicit conversion to get the validated value.
     * 
     * @tparam T The type of value being enforced
     * @tparam Predicate The predicate policy (default: truth)
     * @tparam Raiser The raiser policy (default: default_raiser)
     */
    template<typename T, typename Predicate = predicates::truth, 
             typename Raiser = raisers::default_raiser>
    class enforcer {
    public:
        /**
         * @brief Constructor for simple enforcement
         * @param value The value to enforce
         * @param passed Whether the predicate passed
         * @param expr String representation of the expression
         * @param file Source file name
         * @param line Source line number
         */
        enforcer(T value, bool passed, const char* expr, 
                const char* file, int line)
            : value_(std::move(value))
            , expr_(expr)
            , file_(file)
            , line_(line)
            , failed_(!passed) {
        }
        
        /**
         * @brief Constructor with predicate instance
         * @param value The value to enforce
         * @param passed Whether the predicate passed
         * @param pred The predicate instance
         * @param expr String representation of the expression
         * @param file Source file name
         * @param line Source line number
         */
        enforcer(T value, bool passed, const Predicate& pred, 
                const char* expr, const char* file, int line)
            : value_(std::move(value))
            , predicate_(pred)
            , expr_(expr)
            , file_(file)
            , line_(line)
            , failed_(!passed) {
        }
        
        /**
         * @brief Destructor - triggers enforcement if no custom message provided
         * @throws Exception type determined by Raiser if enforcement failed
         */
        ~enforcer() noexcept(false) {
            if (failed_ && !handled_) {
                // Mark as handled to prevent double-throw
                handled_ = true;
                if constexpr (std::is_same_v<Predicate, predicates::truth>) {
                    Raiser::raise(file_, line_, "Enforcement failed: ", expr_, 
                                 " - ", Predicate::description());
                } else {
                    Raiser::raise(file_, line_, "Enforcement failed: ", expr_, 
                                 " - ", predicate_.description());
                }
            }
        }
        
        /**
         * @brief Provide custom error message
         * 
         * Call operator allows chaining to provide a custom error message.
         * 
         * @tparam Args Message argument types
         * @param args Arguments for error message formatting
         * @return Reference to this enforcer
         * @throws Exception immediately with custom message if enforcement failed
         */
        template<typename... Args>
        enforcer& operator()(Args&&... args) {
            if (failed_) {
                handled_ = true;
                Raiser::raise(file_, line_, std::forward<Args>(args)...);
            }
            return *this;
        }
        
        /**
         * @brief Implicit conversion to the enforced value
         * @return The validated value
         */
        operator T() const {
            return value_;
        }
        
        /**
         * @brief Get the value explicitly
         * @return Reference to the validated value
         */
        T& get() { return value_; }
        
        /**
         * @brief Get the value explicitly (const)
         * @return Const reference to the validated value
         */
        const T& get() const { return value_; }
        
    private:
        T value_;
        Predicate predicate_;
        const char* expr_;
        const char* file_;
        int line_;
        bool failed_;
        bool handled_ = false;
    };
    
    /**
     * @brief Enforcer specialization for expected-like types
     *
     * Unwraps the expected, storing the value on success and the error on failure.
     * The error is used as the default failure message.
     *
     * @tparam V The value type (unwrapped from expected)
     * @tparam E The error type
     * @tparam Raiser The raiser policy
     */
    template<typename V, typename E, typename Raiser = raisers::default_raiser>
    class expected_enforcer {
    public:
        expected_enforcer(V value, bool passed, const char* expr,
                          const char* file, int line)
            : value_(std::move(value))
            , expr_(expr)
            , file_(file)
            , line_(line)
            , failed_(!passed) {
        }

        expected_enforcer(E error, const char* expr,
                          const char* file, int line)
            : expr_(expr)
            , file_(file)
            , line_(line)
            , failed_(true)
            , error_(std::move(error)) {
        }

        ~expected_enforcer() noexcept(false) {
            if (failed_ && !handled_) {
                handled_ = true;
                Raiser::raise(file_, line_, "Enforcement failed: ", expr_,
                              " - error: ", error_);
            }
        }

        template<typename... Args>
        expected_enforcer& operator()(Args&&... args) {
            if (failed_) {
                handled_ = true;
                Raiser::raise(file_, line_, std::forward<Args>(args)...);
            }
            return *this;
        }

        operator V() const { return value_; }
        V& get() { return value_; }
        const V& get() const { return value_; }

    private:
        V value_{};
        const char* expr_;
        const char* file_;
        int line_;
        bool failed_;
        bool handled_ = false;
        E error_{};
    };

    /**
     * @brief Non-owning enforcer for expected-like LVALUE arguments.
     *
     * When ENFORCE receives a named expected (an lvalue), the value must NOT be
     * moved out of the caller's object: doing so leaves the caller with
     * has_value()==true but a moved-from (e.g. null) value, silently breaking
     * everything downstream. This variant references the value in place — no
     * move, no copy — and only validates / raises. Rvalue expecteds (temporaries,
     * or explicit std::move) still use @ref expected_enforcer, which owns the
     * moved-out value so `auto v = ENFORCE(fn())` keeps working.
     *
     * @tparam V The value type (unwrapped from expected)
     * @tparam E The error type
     * @tparam Raiser The raiser policy
     */
    template<typename V, typename E, typename Raiser = raisers::default_raiser>
    class expected_ref_enforcer {
    public:
        expected_ref_enforcer(V* value, const char* expr,
                              const char* file, int line)
            : value_(value)
            , expr_(expr)
            , file_(file)
            , line_(line)
            , failed_(false) {
        }

        expected_ref_enforcer(E error, const char* expr,
                              const char* file, int line)
            : expr_(expr)
            , file_(file)
            , line_(line)
            , failed_(true)
            , error_(std::move(error)) {
        }

        ~expected_ref_enforcer() noexcept(false) {
            if (failed_ && !handled_) {
                handled_ = true;
                Raiser::raise(file_, line_, "Enforcement failed: ", expr_,
                              " - error: ", error_);
            }
        }

        template<typename... Args>
        expected_ref_enforcer& operator()(Args&&... args) {
            if (failed_) {
                handled_ = true;
                Raiser::raise(file_, line_, std::forward<Args>(args)...);
            }
            return *this;
        }

        operator V() const { return *value_; }
        V& get() { return *value_; }
        const V& get() const { return *value_; }

    private:
        V* value_ = nullptr;
        const char* expr_;
        const char* file_;
        int line_;
        bool failed_;
        bool handled_ = false;
        E error_{};
    };

    /**
     * @internal
     * @defgroup EnforcerFactories Factory Functions
     * @{
     */
    
    /**
     * @brief Create basic enforcer with truth predicate
     * @internal
     *
     * For expected-like types, unwraps the value and uses the error
     * as the default failure message.
     */
    template<typename T>
    auto make_enforcer(T&& value, const char* expr, const char* file, int line) {
#if FAILSAFE_HAS_CONCEPTS
        if constexpr (expected_like<std::decay_t<T>>) {
#else
        if constexpr (expected_like_v<std::decay_t<T>>) {
#endif
            using value_type = std::decay_t<decltype(*value)>;
            using error_type = std::decay_t<decltype(value.error())>;
            if constexpr (std::is_lvalue_reference_v<T>) {
                // lvalue: reference the value in place, never move it out of the caller
                if (value.has_value()) {
                    return expected_ref_enforcer<value_type, error_type>(
                        &*value, expr, file, line);
                } else {
                    return expected_ref_enforcer<value_type, error_type>(
                        value.error(), expr, file, line);
                }
            } else {
                if (value.has_value()) {
                    return expected_enforcer<value_type, error_type>(
                        std::move(*value), true, expr, file, line);
                } else {
                    return expected_enforcer<value_type, error_type>(
                        std::move(value.error()), expr, file, line);
                }
            }
        } else {
            bool passed = predicates::truth::check(value);
            return enforcer<std::decay_t<T>>(
                std::forward<T>(value), passed, expr, file, line);
        }
    }
    
    /**
     * @brief Create enforcer with specific exception type
     * @internal
     */
    template<typename Exception, typename T>
    auto make_enforcer_throw(T&& value, const char* expr, const char* file, int line) {
#if FAILSAFE_HAS_CONCEPTS
        if constexpr (expected_like<std::decay_t<T>>) {
#else
        if constexpr (expected_like_v<std::decay_t<T>>) {
#endif
            using value_type = std::decay_t<decltype(*value)>;
            using error_type = std::decay_t<decltype(value.error())>;
            if constexpr (std::is_lvalue_reference_v<T>) {
                // lvalue: reference the value in place, never move it out of the caller
                if (value.has_value()) {
                    return expected_ref_enforcer<value_type, error_type,
                                                 raisers::exception_raiser<Exception>>(
                        &*value, expr, file, line);
                } else {
                    return expected_ref_enforcer<value_type, error_type,
                                                 raisers::exception_raiser<Exception>>(
                        value.error(), expr, file, line);
                }
            } else {
                if (value.has_value()) {
                    return expected_enforcer<value_type, error_type,
                                             raisers::exception_raiser<Exception>>(
                        std::move(*value), true, expr, file, line);
                } else {
                    return expected_enforcer<value_type, error_type,
                                             raisers::exception_raiser<Exception>>(
                        std::move(value.error()), expr, file, line);
                }
            }
        } else {
            bool passed = predicates::truth::check(value);
            return enforcer<std::decay_t<T>, predicates::truth,
                           raisers::exception_raiser<Exception>>(
                std::forward<T>(value), passed, expr, file, line);
        }
    }
    
    /**
     * @brief Create enforcer that traps instead of throwing
     * @internal
     */
    template<typename T>
    auto make_enforcer_trap(T&& value, const char* expr, const char* file, int line) {
#if FAILSAFE_HAS_CONCEPTS
        if constexpr (expected_like<std::decay_t<T>>) {
#else
        if constexpr (expected_like_v<std::decay_t<T>>) {
#endif
            using value_type = std::decay_t<decltype(*value)>;
            using error_type = std::decay_t<decltype(value.error())>;
            if constexpr (std::is_lvalue_reference_v<T>) {
                // lvalue: reference the value in place, never move it out of the caller
                if (value.has_value()) {
                    return expected_ref_enforcer<value_type, error_type, raisers::trap_raiser>(
                        &*value, expr, file, line);
                } else {
                    return expected_ref_enforcer<value_type, error_type, raisers::trap_raiser>(
                        value.error(), expr, file, line);
                }
            } else {
                if (value.has_value()) {
                    return expected_enforcer<value_type, error_type, raisers::trap_raiser>(
                        std::move(*value), true, expr, file, line);
                } else {
                    return expected_enforcer<value_type, error_type, raisers::trap_raiser>(
                        std::move(value.error()), expr, file, line);
                }
            }
        } else {
            bool passed = predicates::truth::check(value);
            return enforcer<std::decay_t<T>, predicates::truth, raisers::trap_raiser>(
                std::forward<T>(value), passed, expr, file, line);
        }
    }
    
    /**
     * @brief Enforce equality
     * @internal
     */
    template<typename T, typename U>
    auto enforce_eq(T&& value, U&& expected, const char* expr, const char* file, int line) {
        predicates::equal_to<std::decay_t<U>> pred{expected};
        bool passed = pred.check(value);
        return enforcer<std::decay_t<T>, predicates::equal_to<std::decay_t<U>>>(
            std::forward<T>(value), passed, pred, expr, file, line);
    }
    
    /**
     * @brief Enforce inequality
     * @internal
     */
    template<typename T, typename U>
    auto enforce_ne(T&& value, U&& expected, const char* expr, const char* file, int line) {
        predicates::not_equal_to<std::decay_t<U>> pred{expected};
        bool passed = pred.check(value);
        return enforcer<std::decay_t<T>, predicates::not_equal_to<std::decay_t<U>>>(
            std::forward<T>(value), passed, pred, expr, file, line);
    }
    
    /**
     * @brief Enforce less than
     * @internal
     */
    template<typename T, typename U>
    auto enforce_lt(T&& value, U&& bound, const char* expr, const char* file, int line) {
        predicates::less_than<std::decay_t<U>> pred{bound};
        bool passed = pred.check(value);
        return enforcer<std::decay_t<T>, predicates::less_than<std::decay_t<U>>>(
            std::forward<T>(value), passed, pred, expr, file, line);
    }
    
    /**
     * @brief Enforce greater than
     * @internal
     */
    template<typename T, typename U>
    auto enforce_gt(T&& value, U&& bound, const char* expr, const char* file, int line) {
        predicates::greater_than<std::decay_t<U>> pred{bound};
        bool passed = pred.check(value);
        return enforcer<std::decay_t<T>, predicates::greater_than<std::decay_t<U>>>(
            std::forward<T>(value), passed, pred, expr, file, line);
    }
    
    /**
     * @brief Enforce in range
     * @internal
     */
    template<typename T, typename L, typename U>
    auto enforce_in_range(T&& value, L&& lower, U&& upper, 
                         const char* expr, const char* file, int line) {
        predicates::in_range<std::decay_t<L>, std::decay_t<U>> pred{lower, upper};
        bool passed = pred.check(value);
        return enforcer<std::decay_t<T>, predicates::in_range<std::decay_t<L>, std::decay_t<U>>>(
            std::forward<T>(value), passed, pred, expr, file, line);
    }
    
    /** @} */ // end of EnforcerFactories group
    
} // namespace failsafe::enforce

namespace failsafe::detail {
    /**
     * @brief Helper function to check if an index is valid
     * 
     * For unsigned types, only checks upper bound.
     * For signed types, checks both bounds.
     */
    template<typename IndexT, typename SizeT>
    inline constexpr bool is_valid_index(IndexT index, SizeT size) noexcept {
        if constexpr (std::is_unsigned_v<IndexT>) {
            return index < size;
        } else {
            return index >= 0 && static_cast<std::make_unsigned_t<IndexT>>(index) < size;
        }
    }
} // namespace failsafe::detail

// Select the plain or message form without optional-comma extensions. The
// public macros take only ... so calls without a message are valid in C++17.
// Each dispatch supports up to 64 arguments, including the checked operands.
// Keeping the message as a call on the enforcer preserves C++17 sequencing:
// the checked operation completes before diagnostic arguments are evaluated.
// Expand argument tuples in separate steps for MSVC's traditional preprocessor.
#define FAILSAFE_DETAIL_ENFORCE_CALL(macro, args) macro args
#define FAILSAFE_DETAIL_ENFORCE_SELECT_ARGS(args) FAILSAFE_DETAIL_ENFORCE_SELECT args
#define FAILSAFE_DETAIL_ENFORCE_SELECT(\
    _1, _2, _3, _4, _5, _6, _7, _8, \
    _9, _10, _11, _12, _13, _14, _15, _16, \
    _17, _18, _19, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _40, \
    _41, _42, _43, _44, _45, _46, _47, _48, \
    _49, _50, _51, _52, _53, _54, _55, _56, \
    _57, _58, _59, _60, _61, _62, _63, _64, \
    selected, ...) selected

#define FAILSAFE_DETAIL_ENFORCE_DISPATCH_1(args) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_1_IMPL args
#define FAILSAFE_DETAIL_ENFORCE_DISPATCH_1_IMPL(plain, message, ...) \
    FAILSAFE_DETAIL_ENFORCE_CALL(FAILSAFE_DETAIL_ENFORCE_SELECT_ARGS((__VA_ARGS__, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, plain, \
        unused)), (__VA_ARGS__))

#define FAILSAFE_DETAIL_ENFORCE_DISPATCH_2(args) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_2_IMPL args
#define FAILSAFE_DETAIL_ENFORCE_DISPATCH_2_IMPL(plain, message, ...) \
    FAILSAFE_DETAIL_ENFORCE_CALL(FAILSAFE_DETAIL_ENFORCE_SELECT_ARGS((__VA_ARGS__, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, plain, plain, \
        unused)), (__VA_ARGS__))

#define FAILSAFE_DETAIL_ENFORCE_DISPATCH_3(args) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_3_IMPL args
#define FAILSAFE_DETAIL_ENFORCE_DISPATCH_3_IMPL(plain, message, ...) \
    FAILSAFE_DETAIL_ENFORCE_CALL(FAILSAFE_DETAIL_ENFORCE_SELECT_ARGS((__VA_ARGS__, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, message, message, message, \
        message, message, message, message, message, plain, plain, plain, \
        unused)), (__VA_ARGS__))

/**
 * @defgroup EnforceMacros Enforcement Macros
 * @{
 */

/**
 * @brief Main enforcement macro
 * 
 * Validates expression and returns the value if true.
 * Throws FAILSAFE_DEFAULT_EXCEPTION on failure.
 * 
 * @param expr Expression to enforce
 * @return The value of expr if validation passes
 * 
 * @example
 * @code
 * auto ptr = ENFORCE(malloc(size));  // Throws on null
 * auto file = ENFORCE(fopen(path, "r"), "Failed to open:", path);
 * @endcode
 */
#define ENFORCE(...) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_1((FAILSAFE_DETAIL_ENFORCE_PLAIN, FAILSAFE_DETAIL_ENFORCE_MESSAGE, __VA_ARGS__))
#define FAILSAFE_DETAIL_ENFORCE_PLAIN(expr) \
    ::failsafe::enforce::make_enforcer((expr), #expr, __FILE__, __LINE__)
#define FAILSAFE_DETAIL_ENFORCE_MESSAGE(expr, ...) \
    FAILSAFE_DETAIL_ENFORCE_PLAIN(expr)(__VA_ARGS__)

/**
 * @brief Enforce with specific exception type
 * 
 * @param expr Expression to enforce
 * @param ExceptionType Exception type to throw on failure
 */
#define ENFORCE_THROW(...) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_2((FAILSAFE_DETAIL_ENFORCE_THROW_PLAIN, FAILSAFE_DETAIL_ENFORCE_THROW_MESSAGE, __VA_ARGS__))
#define FAILSAFE_DETAIL_ENFORCE_THROW_PLAIN(expr, ExceptionType) \
    ::failsafe::enforce::make_enforcer_throw<ExceptionType>((expr), #expr, __FILE__, __LINE__)
#define FAILSAFE_DETAIL_ENFORCE_THROW_MESSAGE(expr, ExceptionType, ...) \
    FAILSAFE_DETAIL_ENFORCE_THROW_PLAIN(expr, ExceptionType)(__VA_ARGS__)

/**
 * @brief Enforce that always traps to debugger
 * 
 * @param expr Expression to enforce
 */
#define ENFORCE_TRAP(...) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_1((FAILSAFE_DETAIL_ENFORCE_TRAP_PLAIN, FAILSAFE_DETAIL_ENFORCE_TRAP_MESSAGE, __VA_ARGS__))
#define FAILSAFE_DETAIL_ENFORCE_TRAP_PLAIN(expr) \
    ::failsafe::enforce::make_enforcer_trap((expr), #expr, __FILE__, __LINE__)
#define FAILSAFE_DETAIL_ENFORCE_TRAP_MESSAGE(expr, ...) \
    FAILSAFE_DETAIL_ENFORCE_TRAP_PLAIN(expr)(__VA_ARGS__)

/** @} */ // end of EnforceMacros group

/**
 * @defgroup ComparisonMacros Comparison Enforcement Macros
 * @{
 */

/** @brief Enforce equality */
#define ENFORCE_EQ(...) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_2((FAILSAFE_DETAIL_ENFORCE_EQ_PLAIN, FAILSAFE_DETAIL_ENFORCE_EQ_MESSAGE, __VA_ARGS__))
#define FAILSAFE_DETAIL_ENFORCE_EQ_PLAIN(value, expected) \
    ::failsafe::enforce::enforce_eq((value), (expected), #value " == " #expected, __FILE__, __LINE__)
#define FAILSAFE_DETAIL_ENFORCE_EQ_MESSAGE(value, expected, ...) \
    FAILSAFE_DETAIL_ENFORCE_EQ_PLAIN(value, expected)(__VA_ARGS__)

/** @brief Enforce inequality */
#define ENFORCE_NE(...) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_2((FAILSAFE_DETAIL_ENFORCE_NE_PLAIN, FAILSAFE_DETAIL_ENFORCE_NE_MESSAGE, __VA_ARGS__))
#define FAILSAFE_DETAIL_ENFORCE_NE_PLAIN(value, expected) \
    ::failsafe::enforce::enforce_ne((value), (expected), #value " != " #expected, __FILE__, __LINE__)
#define FAILSAFE_DETAIL_ENFORCE_NE_MESSAGE(value, expected, ...) \
    FAILSAFE_DETAIL_ENFORCE_NE_PLAIN(value, expected)(__VA_ARGS__)

/** @brief Enforce less than */
#define ENFORCE_LT(...) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_2((FAILSAFE_DETAIL_ENFORCE_LT_PLAIN, FAILSAFE_DETAIL_ENFORCE_LT_MESSAGE, __VA_ARGS__))
#define FAILSAFE_DETAIL_ENFORCE_LT_PLAIN(value, bound) \
    ::failsafe::enforce::enforce_lt((value), (bound), #value " < " #bound, __FILE__, __LINE__)
#define FAILSAFE_DETAIL_ENFORCE_LT_MESSAGE(value, bound, ...) \
    FAILSAFE_DETAIL_ENFORCE_LT_PLAIN(value, bound)(__VA_ARGS__)

/** @brief Enforce greater than */
#define ENFORCE_GT(...) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_2((FAILSAFE_DETAIL_ENFORCE_GT_PLAIN, FAILSAFE_DETAIL_ENFORCE_GT_MESSAGE, __VA_ARGS__))
#define FAILSAFE_DETAIL_ENFORCE_GT_PLAIN(value, bound) \
    ::failsafe::enforce::enforce_gt((value), (bound), #value " > " #bound, __FILE__, __LINE__)
#define FAILSAFE_DETAIL_ENFORCE_GT_MESSAGE(value, bound, ...) \
    FAILSAFE_DETAIL_ENFORCE_GT_PLAIN(value, bound)(__VA_ARGS__)

/** @brief Enforce less than or equal */
#define ENFORCE_LE(...) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_2((FAILSAFE_DETAIL_ENFORCE_LE_PLAIN, FAILSAFE_DETAIL_ENFORCE_LE_MESSAGE, __VA_ARGS__))
#define FAILSAFE_DETAIL_ENFORCE_LE_PLAIN(value, bound) \
    FAILSAFE_DETAIL_ENFORCE_PLAIN((value) <= (bound))
#define FAILSAFE_DETAIL_ENFORCE_LE_MESSAGE(value, bound, ...) \
    FAILSAFE_DETAIL_ENFORCE_LE_PLAIN(value, bound)(__VA_ARGS__)

/** @brief Enforce greater than or equal */
#define ENFORCE_GE(...) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_2((FAILSAFE_DETAIL_ENFORCE_GE_PLAIN, FAILSAFE_DETAIL_ENFORCE_GE_MESSAGE, __VA_ARGS__))
#define FAILSAFE_DETAIL_ENFORCE_GE_PLAIN(value, bound) \
    FAILSAFE_DETAIL_ENFORCE_PLAIN((value) >= (bound))
#define FAILSAFE_DETAIL_ENFORCE_GE_MESSAGE(value, bound, ...) \
    FAILSAFE_DETAIL_ENFORCE_GE_PLAIN(value, bound)(__VA_ARGS__)

/** @brief Enforce value in range [lower, upper] */
#define ENFORCE_IN_RANGE(...) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_3((FAILSAFE_DETAIL_ENFORCE_IN_RANGE_PLAIN, FAILSAFE_DETAIL_ENFORCE_IN_RANGE_MESSAGE, __VA_ARGS__))
#define FAILSAFE_DETAIL_ENFORCE_IN_RANGE_PLAIN(value, lower, upper) \
    ::failsafe::enforce::enforce_in_range((value), (lower), (upper), \
        #value " in [" #lower ", " #upper "]", __FILE__, __LINE__)
#define FAILSAFE_DETAIL_ENFORCE_IN_RANGE_MESSAGE(value, lower, upper, ...) \
    FAILSAFE_DETAIL_ENFORCE_IN_RANGE_PLAIN(value, lower, upper)(__VA_ARGS__)

/** @} */ // end of ComparisonMacros group

/**
 * @defgroup SpecializedMacros Specialized Enforcement Macros
 * @{
 */

/**
 * @brief Enforce non-null pointer
 * 
 * @param ptr Pointer to check
 * @example
 * @code
 * auto buffer = ENFORCE_NOT_NULL(malloc(size));
 * @endcode
 */
#define ENFORCE_NOT_NULL(...) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_1((FAILSAFE_DETAIL_ENFORCE_NOT_NULL_PLAIN, FAILSAFE_DETAIL_ENFORCE_NOT_NULL_MESSAGE, __VA_ARGS__))
#define FAILSAFE_DETAIL_ENFORCE_NOT_NULL_PLAIN(ptr) \
    FAILSAFE_DETAIL_ENFORCE_PLAIN(ptr)("Null pointer: " #ptr)
#define FAILSAFE_DETAIL_ENFORCE_NOT_NULL_MESSAGE(ptr, ...) \
    FAILSAFE_DETAIL_ENFORCE_PLAIN(ptr)(__VA_ARGS__)

/**
 * @brief Enforce valid array index
 * 
 * @param index Index to validate
 * @param size Array size
 * 
 * For unsigned types, only checks upper bound. For signed types, checks both bounds.
 */
#define ENFORCE_VALID_INDEX(...) \
    FAILSAFE_DETAIL_ENFORCE_DISPATCH_2((FAILSAFE_DETAIL_ENFORCE_VALID_INDEX_PLAIN, FAILSAFE_DETAIL_ENFORCE_VALID_INDEX_MESSAGE, __VA_ARGS__))
#define FAILSAFE_DETAIL_ENFORCE_VALID_INDEX_PLAIN(index, size) \
    FAILSAFE_DETAIL_ENFORCE_PLAIN(::failsafe::detail::is_valid_index(index, size))("Index out of bounds: ", (index), " not in [0, ", (size), ")")
#define FAILSAFE_DETAIL_ENFORCE_VALID_INDEX_MESSAGE(index, size, ...) \
    FAILSAFE_DETAIL_ENFORCE_PLAIN(::failsafe::detail::is_valid_index(index, size))(__VA_ARGS__)

/**
 * @brief Debug-only enforcement
 * 
 * Like assert() but with better error messages.
 * Compiled out in release builds.
 * 
 * @param expr Expression to enforce in debug builds
 */
#ifdef NDEBUG
    #define DEBUG_ENFORCE(...) ((void)0)
#else
    #define DEBUG_ENFORCE(...) ENFORCE_TRAP(__VA_ARGS__)
#endif

/** @} */ // end of SpecializedMacros group
