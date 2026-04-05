#pragma once
#include "IWindow.h"

namespace Nalta
{
    class InputSystem;
    
    using OnWindowDestroyedCallback = std::function<void(WindowKey)>;

    class IPlatformSystem
    {
    public:
        virtual ~IPlatformSystem() = default;

        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual bool PollEvents() = 0; // Returns false if wants to close main window
        virtual void TickInput() = 0;

        [[nodiscard]] virtual WindowKey CreatePlatformWindow(const WindowDesc& aDesc) = 0;
        [[nodiscard]] virtual IWindow* GetWindow(WindowKey aKey) = 0;
        virtual void DestroyWindow(WindowKey aWindow) = 0;
        
        [[nodiscard]] virtual InputSystem& GetInputSystem() = 0;
        
        virtual void SetOnWindowDestroyedCallback(OnWindowDestroyedCallback aCallback) = 0;
        
        virtual void SetCurrentThreadName(const std::string& aName) const = 0;

        [[nodiscard]] virtual uint32_t GetCPUCoreCount() const = 0;
        [[nodiscard]] virtual uint64_t GetSystemMemoryBytes() const = 0;
    };
}