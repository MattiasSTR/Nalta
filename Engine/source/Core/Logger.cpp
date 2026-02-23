#include "Nalta/Core/Logger.h"

#ifndef N_SHIPPING

#include <iostream>
#include <memory>
#include <stack>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Nalta
{
    Logger* GLogger = nullptr;
    
    struct Logger::Impl
    {
        std::shared_ptr<spdlog::logger> logger;
        Level currentLevel{ Level::Trace };
        std::mutex logMutex;
        
        thread_local static std::stack<std::string> ourThreadScopeStack;

        // stdout/stderr redirection
        class StdRedirect : public std::streambuf
        {
        public:
            explicit StdRedirect(const Level aLevel) : myLevel(aLevel) {}

        protected:
            int_type overflow(const int_type aCh) override
            {
                if (aCh != traits_type::eof())
                {
                    myBuffer += static_cast<char>(aCh);
                    if (aCh == '\n')
                    {
                        const std::lock_guard lock(ourMutex);
                        if (GLogger != nullptr) // use global pointer
                        {
                            GLogger->Log(myLevel, myBuffer);
                        }
                        myBuffer.clear();
                    }
                }
                return aCh;
            }

        private:
            std::string myBuffer;
            Level myLevel;
            static inline std::mutex ourMutex;
        };

        StdRedirect* coutRedirect{ nullptr };
        StdRedirect* cerrRedirect{ nullptr };
    };
    
    thread_local std::stack<std::string, std::deque<std::string>> Logger::Impl::ourThreadScopeStack;

    Logger::Logger() : myImpl(new Impl) {}
    Logger::~Logger()
    {
        Shutdown(); 
        delete myImpl; 
        myImpl = nullptr;
    }

    void Logger::Init() const
    {
        auto& impl{ *myImpl };
        const auto consoleSink{ std::make_shared<spdlog::sinks::stdout_color_sink_mt>() };
        const auto fileSink{ std::make_shared<spdlog::sinks::basic_file_sink_mt>("Nalta.log", true) };
        std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };

        impl.logger = std::make_shared<spdlog::logger>("NALTA", sinks.begin(), sinks.end());
        //impl.logger->set_pattern("[%H:%M:%S:%e] [%^%l%$] %v");
        impl.logger->set_pattern("[%H:%M:%S:%e] [%^%L%$] [Thread %t] %v");
        impl.logger->set_level(spdlog::level::trace);

        impl.coutRedirect = new Impl::StdRedirect(Level::Info);
        impl.cerrRedirect = new Impl::StdRedirect(Level::Error);
        
        std::cout.rdbuf(impl.coutRedirect);
        std::cerr.rdbuf(impl.cerrRedirect);
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
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            {
                msg += "[" + *it + "] ";
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
    void Logger::Init() const {}
    void Logger::Shutdown() const {}
    void Logger::SetLevel(Level) const {}
    Logger::Level Logger::GetLevel() const { return Level::Off; }
    void Logger::Log(Level, std::string_view) const {}
    void Logger::Fatal(std::string_view) const { std::terminate(); }
    void Logger::PushScope(const std::string&) const {}
    void Logger::PopScope() const {}
}

#endif