#pragma once

#include <cstdint>
#include <format>
#include <string_view>

namespace Nalta::Logger
{
    enum class Level : uint8_t
    {
        Trace,
        Info,
        Warn,
        Error,
        Critical,
        Off
    };

    void Init();
    void Shutdown();

    void SetLevel(Level aLevel);
    Level GetLevel();

    void LogInternal(Level aLevel, std::string_view aMessage);

    template<typename... Args>
    void Log(Level aLevel,std::format_string<Args...> aFormat, Args&&... aArgs)
    {
#ifndef _SHIPPING
        if (aLevel < GetLevel())
        {
            return;
        }

        auto message = std::format(aFormat, std::forward<Args>(aArgs)...);
        LogInternal(aLevel, message);
#else
        (void)aLevel;
        (void)aFormat;
#endif
    }
    
    [[noreturn]] void Fatal(std::string_view aMessage);
}

#ifndef _SHIPPING

#define NL_TRACE(...)    ::Nalta::Logger::Log(::Nalta::Logger::Level::Trace, __VA_ARGS__)
#define NL_INFO(...)     ::Nalta::Logger::Log(::Nalta::Logger::Level::Info,  __VA_ARGS__)
#define NL_WARN(...)     ::Nalta::Logger::Log(::Nalta::Logger::Level::Warn,  __VA_ARGS__)
#define NL_ERROR(...)    ::Nalta::Logger::Log(::Nalta::Logger::Level::Error, __VA_ARGS__)
#define NL_CRITICAL(...) ::Nalta::Logger::Log(::Nalta::Logger::Level::Critical, __VA_ARGS__)
#define NL_FATAL(msg, ...) \
    do { \
    ::Nalta::Logger::Fatal(std::format(msg, ##__VA_ARGS__)); \
    } while(0)

#else

#define NL_TRACE(...)    ((void)0)
#define NL_INFO(...)     ((void)0)
#define NL_WARN(...)     ((void)0)
#define NL_ERROR(...)    ((void)0)
#define NL_CRITICAL(...) ((void)0)
#define FATAL(msg, ...) ((void)0)

#endif