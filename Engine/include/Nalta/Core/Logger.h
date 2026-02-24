#pragma once
#include <format>
#include <string_view>

namespace Nalta
{
    // For patterns, see https://github.com/gabime/spdlog/wiki/Custom-formatting
    struct LoggerConfig
    {
        std::string name; // Logger name
        std::string fileName; // File sink, leave empty for no sink
        std::string pattern = "%^[%H:%M:%S:%e] [Thread %t] %n:%$ %v";
    };
    
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

        void Init(const LoggerConfig& aConfig) const;
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
    
    extern Logger* GCoreLogger;  // Core engine logger
    extern Logger* GGameLogger;  // Game-specific logger
    
    class LoggerScope
    {
    public:
        explicit LoggerScope(const Logger* aLogger, const std::string& aScope) : myLogger{ aLogger }
        {
            if (myLogger) myLogger->PushScope(aScope);
        }
        ~LoggerScope()
        {
            if (myLogger) myLogger->PopScope();
        }
        
    private:
        const Logger* myLogger{ nullptr };
    };
}

#ifndef N_SHIPPING
#define NL_TRACE(logger, ...)    (logger)->LogFmt(::Nalta::Logger::Level::Trace, __VA_ARGS__)
#define NL_INFO(logger, ...)     (logger)->LogFmt(::Nalta::Logger::Level::Info,  __VA_ARGS__)
#define NL_WARN(logger, ...)     (logger)->LogFmt(::Nalta::Logger::Level::Warn,  __VA_ARGS__)
#define NL_ERROR(logger, ...)    (logger)->LogFmt(::Nalta::Logger::Level::Error, __VA_ARGS__)
#define NL_CRITICAL(logger, ...) (logger)->LogFmt(::Nalta::Logger::Level::Critical, __VA_ARGS__)
#define NL_FATAL(logger, msg, ...) (logger)->Fatal(std::format(msg, ##__VA_ARGS__))
#else
#define NL_TRACE(logger, ...)    ((void)0)
#define NL_INFO(logger, ...)     ((void)0)
#define NL_WARN(logger, ...)     ((void)0)
#define NL_ERROR(logger, ...)    ((void)0)
#define NL_CRITICAL(logger, ...) ((void)0)
#define NL_FATAL(logger, msg, ...) ((void)0)
#endif