#include "npch.h"
#include "Nalta/Core/EngineConfig.h"

namespace Nalta
{
    void ParseEngineConfig(EngineConfig& outConfig, const int aArgc, char** aArgv)
    {
        for (int i = 1; i < aArgc; ++i)
        {
            std::string const arg{ aArgv[i] };

            if (arg == "--headless" || arg == "--server")
            {
                outConfig.mode = EngineMode::Headless;
                outConfig.enableRendering = false;
            }
            else if (arg == "--no-render")
            {
                outConfig.enableRendering = false;
            }
            else if (arg.starts_with("--width="))
            {
                if (outConfig.mainWindowDesc)
                {
                    outConfig.mainWindowDesc->width = std::stoi(arg.substr(8));
                }
            }
            else if (arg.starts_with("--height="))
            {
                if (outConfig.mainWindowDesc)
                {
                    outConfig.mainWindowDesc->height = std::stoi(arg.substr(9));
                }
            }
            else if (arg.starts_with("--title="))
            {
                if (outConfig.mainWindowDesc)
                {
                    outConfig.mainWindowDesc->caption = arg.substr(8);
                }
            }
        }
    }

    EngineConfig GetDefaultGameConfig(const std::string& aTitle)
    {
        EngineConfig config;
        config.mode             = EngineMode::Client;
        config.enableRendering  = true;
        config.mainWindowDesc           = WindowDesc{.width = 1280, .height = 720, .caption = aTitle, .isMainWindow = true };
        config.fixedDelta       = 1.0 / 50.0;

        config.coreLogger.pattern   = "%^[%H:%M:%S:%e] [Thread %t] %n:%$ %v";
        config.coreLogger.fileName  = "Nalta.log";
        config.coreLogger.name      = "NALTA";
        
        config.gameLogger.name      = "GAME";
        config.gameLogger.pattern   = "%^%n:%$ %v";

        return config;
    }

    EngineConfig GetDefaultServerConfig()
    {
        EngineConfig config;
        config.mode             = EngineMode::Headless;
        config.enableRendering  = false;
        config.fixedDelta       = 1.0 / 50.0;
        config.mainWindowDesc.reset();
        
        config.coreLogger.pattern   = "%^[%H:%M:%S:%e] [Thread %t] %n:%$ %v";
        config.coreLogger.fileName  = "";
        config.coreLogger.name      = "NALTA_SERVER";
        
        config.gameLogger.name      = "";
        config.gameLogger.pattern   = "";
        config.gameLogger.fileName  = "";
        
        return config;
    }
}
