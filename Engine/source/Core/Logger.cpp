#include "Nalta/Core/Logger.h"

#ifndef _SHIPPING

#include <iostream>
#include <memory>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Nalta::Logger
{
    namespace
    {
        std::mutex locLogMutex;
        
        spdlog::level::level_enum ToSpd(const Level aLevel)
        {
            switch (aLevel)
            {
                case Level::Trace:    return spdlog::level::trace;
                case Level::Info:     return spdlog::level::info;
                case Level::Warn:     return spdlog::level::warn;
                case Level::Error:    return spdlog::level::err;
                case Level::Critical: return spdlog::level::critical;
                case Level::Off:      return spdlog::level::off;
                default:              return spdlog::level::info;
            }
        }
        
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
                        std::lock_guard<std::mutex> lock(locLogMutex);
                        LogInternal(myLevel, myBuffer);
                        myBuffer.clear();
                    }
                }
                return aCh;
            }

        private:
            std::string myBuffer;
            Level myLevel;
        };

        StdRedirect* locCoutRedirect{ nullptr };
        StdRedirect* locCerrRedirect{ nullptr };
        
        std::shared_ptr<spdlog::logger> locLogger;
        Level locCurrentLevel{ Level::Trace };
    }

    void Init()
    {
        auto consoleSink{ std::make_shared<spdlog::sinks::stdout_color_sink_mt>() };
        auto fileSink{ std::make_shared<spdlog::sinks::basic_file_sink_mt>("Nalta.log", true) };
        std::vector<spdlog::sink_ptr> sinks { consoleSink, fileSink };
        locLogger = std::make_shared<spdlog::logger>("NALTA", sinks.begin(), sinks.end());
        
        locLogger->set_pattern("[%H:%M:%S:%e] [Thread %t] [Process %P] [%^%l%$] %v");
        locLogger->set_level(spdlog::level::trace);

        spdlog::register_logger(locLogger);
        
        locCoutRedirect = new StdRedirect(Level::Info);
        std::cout.rdbuf(locCoutRedirect);

        locCerrRedirect = new StdRedirect(Level::Error);
        std::cerr.rdbuf(locCerrRedirect);
    }

    void Shutdown()
    {
        delete locCoutRedirect;
        delete locCerrRedirect;

        spdlog::shutdown();
        locLogger.reset();
    }

    void SetLevel(const Level aLevel)
    {
        locCurrentLevel = aLevel;

        if (locLogger)
        {
            locLogger->set_level(ToSpd(aLevel));
        }
    }

    Level GetLevel()
    {
        return locCurrentLevel;
    }

    void LogInternal(const Level aLevel, const std::string_view aMessage)
    {
        if (!locLogger)
        {
            return;
        }

        locLogger->log(ToSpd(aLevel), aMessage);
    }

    void Fatal(const std::string_view aMessage)
    {
        // Ensure logger exists
        if (locLogger)
        {
            LogInternal(Level::Critical, aMessage);
            locLogger->flush();
        }
        else
        {
            std::cerr << "[FATAL] " << aMessage << "\n";
        }

#ifdef _WIN32
        MessageBoxA(nullptr, std::string(aMessage).c_str(), "FATAL ERROR", MB_OK | MB_ICONERROR);
#endif

        std::terminate();
    }
}
#else

namespace Nalta::Logger
{
    void Init() {}
    void Shutdown() {}
    void SetLevel(Level) {}
    Level GetLevel() { return Level::Off; }
    void LogInternal(Level, std::string_view) {}
}

#endif