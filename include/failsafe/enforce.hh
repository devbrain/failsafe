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
     * @internal
     * @defgroup EnforcerFactories Factory Functions
     * @{
     */
    
    /**
     * @brief Create basic enforcer with truth predicate
     * @internal
     */
    template<typename T>
    auto make_enforcer(T&& value, const char* expr, const char* file, int line) {
        bool passed = predicates::truth::check(value);
        return enforcer<std::decay_t<T>>(
            std::forward<T>(value), passed, expr, file, line);
    }
    
    /**
     * @brief Create enforcer with specific exception type
     * @internal
     */
    template<typename Exception, typename T>
    auto make_enforcer_throw(T&& value, const char* expr, const char* file, int line) {
        bool passed = predicates::truth::check(value);
        return enforcer<std::decay_t<T>, predicates::truth, 
                       raisers::exception_raiser<Exception>>(
            std::forward<T>(value), passed, expr, file, line);
    }
    
    /**
     * @brief Create enforcer that traps instead of throwing
     * @internal
     */
    template<typename T>
    auto make_enforcer_trap(T&& value, const char* expr, const char* file, int line) {
        bool passed = predicates::truth::check(value);
        return enforcer<std::decay_t<T>, predicates::truth, raisers::trap_raiser>(
            std::forward<T>(value), passed, expr, file, line);
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
 * auto file = ENFORCE(fopen(path, "r"))("Failed to open:", path);
 * @endcode
 */
#define ENFORCE(expr) \
    ::failsafe::enforce::make_enforcer((expr), #expr, __FILE__, __LINE__)

/**
 * @brief Enforce with specific exception type
 * 
 * @param expr Expression to enforce
 * @param ExceptionType Exception type to throw on failure
 */
#define ENFORCE_THROW(expr, ExceptionType) \
    ::failsafe::enforce::make_enforcer_throw<ExceptionType>((expr), #expr, __FILE__, __LINE__)

/**
 * @brief Enforce that always traps to debugger
 * 
 * @param expr Expression to enforce
 */
#define ENFORCE_TRAP(expr) \
    ::failsafe::enforce::make_enforcer_trap((expr), #expr, __FILE__, __LINE__)

/** @} */ // end of EnforceMacros group

/**
 * @defgroup ComparisonMacros Comparison Enforcement Macros
 * @{
 */

/** @brief Enforce equality */
#define ENFORCE_EQ(value, expected) \
    ::failsafe::enforce::enforce_eq((value), (expected), #value " == " #expected, __FILE__, __LINE__)

/** @brief Enforce inequality */
#define ENFORCE_NE(value, expected) \
    ::failsafe::enforce::enforce_ne((value), (expected), #value " != " #expected, __FILE__, __LINE__)

/** @brief Enforce less than */
#define ENFORCE_LT(value, bound) \
    ::failsafe::enforce::enforce_lt((value), (bound), #value " < " #bound, __FILE__, __LINE__)

/** @brief Enforce greater than */
#define ENFORCE_GT(value, bound) \
    ::failsafe::enforce::enforce_gt((value), (bound), #value " > " #bound, __FILE__, __LINE__)

/** @brief Enforce less than or equal */
#define ENFORCE_LE(value, bound) \
    ENFORCE((value) <= (bound))

/** @brief Enforce greater than or equal */
#define ENFORCE_GE(value, bound) \
    ENFORCE((value) >= (bound))

/** @brief Enforce value in range [lower, upper] */
#define ENFORCE_IN_RANGE(value, lower, upper) \
    ::failsafe::enforce::enforce_in_range((value), (lower), (upper), \
        #value " in [" #lower ", " #upper "]", __FILE__, __LINE__)

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
#define ENFORCE_NOT_NULL(ptr) \
    ENFORCE(ptr)("Null pointer: " #ptr)

/**
 * @brief Enforce valid array index
 * 
 * @param index Index to validate
 * @param size Array size
 */
#define ENFORCE_VALID_INDEX(index, size) \
    ENFORCE((index) >= 0 && (index) < (size))("Index out of bounds: ", (index), " not in [0, ", (size), ")")

/**
 * @brief Debug-only enforcement
 * 
 * Like assert() but with better error messages.
 * Compiled out in release builds.
 * 
 * @param expr Expression to enforce in debug builds
 */
#ifdef NDEBUG
    #define DEBUG_ENFORCE(expr) ((void)0)
#else
    #define DEBUG_ENFORCE(expr) ENFORCE_TRAP(expr)
#endif

/** @} */ // end of SpecializedMacros group