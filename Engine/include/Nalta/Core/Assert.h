#pragma once
#include "Nalta/Core/Logger.h"

#include <filesystem>
#include <format>
#include <string_view>

#ifdef N_ENABLE_ASSERTS

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
            NL_CRITICAL(::Nalta::GGameLogger, \
                "[Assert] {} ({}:{}): " msg, \
                N_STRINGIFY_MACRO(check), \
                std::filesystem::path(__FILE__).filename().string(), \
                __LINE__, ##__VA_ARGS__); \
            N_DEBUGBREAK(); \
        } \
    } while(0)

#define N_CORE_INTERNAL_ASSERT_IMPL(check, msg, ...) \
    do { \
        if (!(check)) { \
            NL_CRITICAL(::Nalta::GCoreLogger, \
                "[Assert] {} ({}:{}): " msg, \
                N_STRINGIFY_MACRO(check), \
                std::filesystem::path(__FILE__).filename().string(), \
                __LINE__, ##__VA_ARGS__); \
        N_DEBUGBREAK(); \
        } \
    } while(0)

#define N_ASSERT(check, msg, ...)      N_INTERNAL_ASSERT_IMPL(check, msg, ##__VA_ARGS__)
#define N_CORE_ASSERT(check, msg, ...) N_CORE_INTERNAL_ASSERT_IMPL(check, msg, ##__VA_ARGS__)

#else
#define N_ASSERT(check, msg, ...)      ((void)0)
#define N_CORE_ASSERT(check, msg, ...) ((void)0)
#endif