#pragma once
#include "Nalta/Core/Logger.h"
#include "Nalta/Platform/WindowDesc.h"

#include <optional>

namespace Nalta
{
    enum class EngineMode : uint8_t
    {
        Client,      // windowed game client
        Headless,    // used for server or automated testing
    };
    
    struct EngineConfig
    {
        // Core mode
        EngineMode mode{ EngineMode::Client };
        
        // Flags to disable subsystems individually
        bool enableRendering{ true };     // used only if !headless
        
        // Window settings (optional; used if headless==false)
        std::optional<WindowDesc> window;
        
        // Logging settings
        LoggerConfig coreLogger;         // Core engine logger
        LoggerConfig gameLogger;         // Game-specific logger
        
        // Fixed timestep for updates
        double fixedDelta{ 1.0 / 50.0 };

        [[nodiscard]] bool IsServer() const { return mode == EngineMode::Headless; }
        [[nodiscard]] bool IsClient() const { return mode == EngineMode::Client; }
        
        [[nodiscard]] bool ShouldCreateWindow() const
        {
            return IsClient() && enableRendering && window.has_value();
        }
        
        [[nodiscard]] bool ShouldRunRenderThread() const
        {
            return IsClient() && enableRendering;
        }
    };

    void ParseEngineConfig(EngineConfig& outConfig, int aArgc, char** aArgv);
    EngineConfig GetDefaultGameConfig(const std::string& aTitle = "Nalta");
    EngineConfig GetDefaultServerConfig();
}
