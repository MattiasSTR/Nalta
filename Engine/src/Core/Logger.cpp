#include "npch.h"
#include "Nalta/Core/Logger.h"

namespace Nalta
{
    Logger* GCoreLogger = nullptr;
    Logger* GGameLogger = nullptr;
}

#ifndef N_SHIPPING

#include <iostream>
#include <memory>
#include <ranges>
#include <stack>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Nalta
{
    struct Logger::Impl
    {
        std::shared_ptr<spdlog::logger> logger;
        Level currentLevel{ Level::Trace };
        std::mutex logMutex;
        
        inline thread_local static std::stack<std::string> ourThreadScopeStack;
    };

    Logger::Logger() : myImpl(new Impl) {}
    Logger::~Logger()
    {
        Shutdown(); 
        delete myImpl; 
        myImpl = nullptr;
    }

    void Logger::Init(const std::string& aName) const
    {
        auto& impl{ *myImpl };
        const auto consoleSink{ std::make_shared<spdlog::sinks::stdout_color_sink_mt>() };
        const auto fileSink{ std::make_shared<spdlog::sinks::basic_file_sink_mt>("Nalta.log", true) };
        std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };

        impl.logger = std::make_shared<spdlog::logger>(aName, sinks.begin(), sinks.end());
        //impl.logger->set_pattern("[%H:%M:%S:%e] [%^%l%$] %v");
        //impl.logger->set_pattern("[%H:%M:%S:%e] [%^%L%$] [Thread %t] %v");
        impl.logger->set_pattern("%^[%H:%M:%S:%e] [Thread %t] %n:%$ %v");
        impl.logger->set_level(spdlog::level::trace);
        impl.logger->flush_on(spdlog::level::trace);
    }

    void Logger::Shutdown() const
    {
        if (myImpl == nullptr)
        {
            return;
        }
        spdlog::shutdown();
        myImpl->logger.reset();
    }

    void Logger::SetLevel(const Level aLevel) const
    {
        myImpl->currentLevel = aLevel;
        
        if (myImpl->logger)
        {
            // Map to spdlog levels
            spdlog::level::level_enum lvl{ spdlog::level::info };
            switch (aLevel)
            {
                case Level::Trace: lvl = spdlog::level::trace; break;
                case Level::Info: lvl = spdlog::level::info; break;
                case Level::Warn: lvl = spdlog::level::warn; break;
                case Level::Error: lvl = spdlog::level::err; break;
                case Level::Critical: lvl = spdlog::level::critical; break;
                case Level::Off: lvl = spdlog::level::off; break;
            }
            myImpl->logger->set_level(lvl);
        }
    }

    Logger::Level Logger::GetLevel() const
    {
        return myImpl->currentLevel;
    }

    void Logger::Log(const Level aLevel, const std::string_view aMessage) const
    {
        if (!myImpl->logger)
        {
            return;
        }
        const std::scoped_lock lock(myImpl->logMutex);
        
        std::string msg;
        const auto& stack{ Impl::ourThreadScopeStack };
        if (!stack.empty())
        {
            std::stack<std::string> temp{ stack };
            std::vector<std::string> scopes;
            while (!temp.empty())
            {
                scopes.push_back(temp.top());
                temp.pop();
            }
            // reverse for outer->inner
            for (auto& scope : std::ranges::reverse_view(scopes))
            {
                msg += "[" + scope + "] ";
            }
        }
        msg += aMessage;

        switch (aLevel)
        {
            case Level::Trace:    myImpl->logger->trace(msg); break;
            case Level::Info:     myImpl->logger->info(msg); break;
            case Level::Warn:     myImpl->logger->warn(msg); break;
            case Level::Error:    myImpl->logger->error(msg); break;
            case Level::Critical: myImpl->logger->critical(msg); break;
            default:              myImpl->logger->info(msg); break;
        }
    }

    [[noreturn]] void Logger::Fatal(const std::string_view aMessage) const
    {
        Log(Level::Critical, aMessage);
        if (myImpl->logger)
        {
            myImpl->logger->flush();
        }
        
#ifdef _WIN32
        MessageBoxA(nullptr, std::string(aMessage).c_str(), "FATAL ERROR", MB_OK | MB_ICONERROR);
#endif
        
        std::terminate();
    }

    void Logger::PushScope(const std::string& aScope) const
    {
        Impl::ourThreadScopeStack.push(aScope);
    }

    void Logger::PopScope() const
    {
        if (!Impl::ourThreadScopeStack.empty())
        {
            Impl::ourThreadScopeStack.pop();
        }
    }
}

#else

namespace Nalta
{
    Logger::Logger() : myImpl(nullptr) {}
    Logger::~Logger() {}
    void Logger::Init(const std::string&) const {}
    void Logger::Shutdown() const {}
    void Logger::SetLevel(Level) const {}
    Logger::Level Logger::GetLevel() const { return Level::Off; }
    void Logger::Log(Level, std::string_view) const {}
    void Logger::Fatal(std::string_view) const { std::terminate(); }
    void Logger::PushScope(const std::string&) const {}
    void Logger::PopScope() const {}
}

#endif