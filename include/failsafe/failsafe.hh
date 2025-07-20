/**
 * @file failsafe.hh
 * @brief Main header file that includes all Failsafe components
 * 
 * @details
 * This is a convenience header that includes all the main components
 * of the Failsafe library. For more granular control over compile times
 * and dependencies, you can include individual headers as needed.
 * 
 * @example
 * @code
 * // Include everything
 * #include <failsafe/failsafe.hh>
 * 
 * // Or include only what you need
 * #include <failsafe/logger.hh>
 * #include <failsafe/enforce.hh>
 * @endcode
 */
#pragma once

// Core components
#include <failsafe/logger.hh>
#include <failsafe/enforce.hh>
#include <failsafe/exception.hh>

// String utilities (also included by logger)
#include <failsafe/detail/string_utils.hh>

// Logger backends
#include <failsafe/logger/backend/cerr_backend.hh>

// Optional: Include other backends only if needed
// #include <failsafe/logger/backend/poco_backend.hh>
// #include <failsafe/logger/backend/grpc_logger.hh>