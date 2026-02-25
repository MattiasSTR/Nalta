#pragma once

// Platform detection
#ifdef _WIN32
    #ifdef _WIN64
        #define N_PLATFORM_WINDOWS
    #else
        #error "x86 builds are not supported."
    #endif

#elif defined(__APPLE__) || defined(__MACH__)
    #include <TargetConditionals.h>

    #if TARGET_IPHONE_SIMULATOR == 1
        #error "iOS simulator is not supported."
    #elif TARGET_OS_IPHONE == 1
        #define N_PLATFORM_IOS
        #error "iOS is not supported."
    #elif TARGET_OS_MAC == 1
        #define N_PLATFORM_MACOS
        #error "macOS is not supported."
    #else
        #error "Unknown Apple platform."
    #endif

#elif defined(__ANDROID__)
    #define N_PLATFORM_ANDROID
    #error "Android is not supported."
#elif defined(__linux__)
    #define N_PLATFORM_LINUX
    #error "Linux is not supported."
#else
    #error "Unknown platform."
#endif


// Compiler detection
#ifdef _MSC_VER
    #define N_COMPILER_MSVC
#elif defined(__clang__)
    #define N_COMPILER_CLANG
#elif defined(__GNUC__)
    #define N_COMPILER_GCC
#else
    #error "Unknown compiler."
#endif