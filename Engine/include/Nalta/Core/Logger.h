#pragma once
#include <format>
#include <string_view>

namespace Nalta
{
    class Logger
    {
    public:
        enum class Level : uint8_t
        {
            Trace, 
            Info, 
            Warn, 
            Error, 
            Critical, 
            Off
        };

        Logger();
        ~Logger();

        void Init() const;
        void Shutdown() const;

        void SetLevel(Level aLevel) const;
        Level GetLevel() const;

        void Log(Level aLevel, std::string_view aMessage) const;

        template<typename... Args>
        void LogFmt(const Level aLevel, std::format_string<Args...> aFormat, Args&&... aArgs)
        {
#ifndef N_SHIPPING
            if (aLevel < GetLevel())
            {
                return;
            }
            Log(aLevel, std::format(aFormat, std::forward<Args>(aArgs)...));
#endif
        }
  
        [[noreturn]] void Fatal(std::string_view aMessage) const;
        
        void PushScope(const std::string& aScope) const;
        void PopScope() const;

    private:
        struct Impl;
        Impl* myImpl;
    };
    
    extern Logger* GLogger; // global pointer used by macros
    
    class LoggerScope
    {
    public:
        explicit LoggerScope(const std::string& aScope)
        {
            if (GLogger) GLogger->PushScope(aScope);
        }
        ~LoggerScope()
        {
            if (GLogger) GLogger->PopScope();
        }
    };
}

#ifndef N_SHIPPING
#define NL_TRACE(...)    ::Nalta::GLogger->LogFmt(::Nalta::Logger::Level::Trace, __VA_ARGS__)
#define NL_INFO(...)     ::Nalta::GLogger->LogFmt(::Nalta::Logger::Level::Info,  __VA_ARGS__)
#define NL_WARN(...)     ::Nalta::GLogger->LogFmt(::Nalta::Logger::Level::Warn,  __VA_ARGS__)
#define NL_ERROR(...)    ::Nalta::GLogger->LogFmt(::Nalta::Logger::Level::Error, __VA_ARGS__)
#define NL_CRITICAL(...) ::Nalta::GLogger->LogFmt(::Nalta::Logger::Level::Critical, __VA_ARGS__)
#define NL_FATAL(msg, ...) ::Nalta::GLogger->Fatal(std::format(msg, ##__VA_ARGS__))
#else
#define NL_TRACE(...)    ((void)0)
#define NL_INFO(...)     ((void)0)
#define NL_WARN(...)     ((void)0)
#define NL_ERROR(...)    ((void)0)
#define NL_CRITICAL(...) ((void)0)
#define NL_FATAL(msg, ...) ((void)0)
#endif