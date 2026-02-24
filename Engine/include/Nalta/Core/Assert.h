#pragma once
#include "Nalta/Core/Logger.h"

#include <filesystem>
#include <format>
#include <string_view>

#ifdef N_ENABLE_ASSERTS

// Cross-platform debug break // TODO: REMOVE WHEN BETTER PLATFORM SYSTEM
#ifdef _WIN64
    #define N_DEBUGBREAK() __debugbreak()
#else
    #include <csignal>
    #define N_DEBUGBREAK() raise(SIGTRAP)
#endif

#define N_EXPAND_MACRO(x) x
#define N_STRINGIFY_MACRO(x) #x

#define N_INTERNAL_ASSERT_IMPL(check, msg, ...) \
    do { \
        if (!(check)) { \
            NL_CRITICAL(::Nalta::GGameLogger, "{} ({}:{}): " msg, \
            N_STRINGIFY_MACRO(check), \
            std::filesystem::path(__FILE__).filename().string(), \
            __LINE__, \
            ##__VA_ARGS__); \
            N_DEBUGBREAK(); \
        } \
    } while(0)

#define N_CORE_INTERNAL_ASSERT_IMPL(check, msg, ...) \
    do { \
        if (!(check)) { \
            NL_CRITICAL(::Nalta::GCoreLogger, "{} ({}:{}): " msg, \
            N_STRINGIFY_MACRO(check), \
            std::filesystem::path(__FILE__).filename().string(), \
            __LINE__, \
            ##__VA_ARGS__); \
            N_DEBUGBREAK(); \
        } \
    } while(0)

// Assert with a custom message
#define N_INTERNAL_ASSERT_WITH_MSG(check, ...) \
    N_INTERNAL_ASSERT_IMPL(check, __VA_ARGS__)

#define N_CORE_INTERNAL_ASSERT_WITH_MSG(check, ...) \
    N_CORE_INTERNAL_ASSERT_IMPL(check, __VA_ARGS__)

// Assert without a custom message
#define N_INTERNAL_ASSERT_NO_MSG(check) \
    N_INTERNAL_ASSERT_IMPL(check, "")

#define N_CORE_INTERNAL_ASSERT_NO_MSG(check) \
    N_CORE_INTERNAL_ASSERT_IMPL(check, "")

// Macro selector (detects if a message was provided)
#define N_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define N_INTERNAL_ASSERT_GET_MACRO(...) \
    N_EXPAND_MACRO(N_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, \
    N_INTERNAL_ASSERT_WITH_MSG, N_INTERNAL_ASSERT_NO_MSG))

#define N_CORE_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define N_CORE_INTERNAL_ASSERT_GET_MACRO(...) \
    N_EXPAND_MACRO(N_CORE_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, \
    N_CORE_INTERNAL_ASSERT_WITH_MSG, N_CORE_INTERNAL_ASSERT_NO_MSG))

// Public assert macro (core only)
#define N_ASSERT(...) N_EXPAND_MACRO(N_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(__VA_ARGS__))
#define N_CORE_ASSERT(...) N_EXPAND_MACRO(N_CORE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(__VA_ARGS__))

#else
#define N_ASSERT(...)
#endif