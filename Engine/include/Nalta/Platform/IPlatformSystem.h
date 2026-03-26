#pragma once
#include "WindowHandle.h"

namespace Nalta
{
    class InputSystem;
    struct WindowDesc;
    
    using OnWindowDestroyedCallback = std::function<void(WindowHandle)>;

    class IPlatformSystem
    {
    public:
        virtual ~IPlatformSystem() = default;

        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual bool PollEvents() = 0; // Returns false if wants to close main window
        virtual void TickInput() = 0;

        [[nodiscard]] virtual WindowHandle CreatePlatformWindow(const WindowDesc& aDesc) = 0;
        virtual void DestroyWindow(WindowHandle aWindow) = 0;
        
        [[nodiscard]] virtual WindowHandle GetMainWindow() const = 0;
        
        [[nodiscard]] virtual InputSystem& GetInputSystem() = 0;
        
        virtual void SetOnWindowDestroyedCallback(OnWindowDestroyedCallback aCallback) = 0;
        
        virtual void SetCurrentThreadName(const std::string& aName) const = 0;

        [[nodiscard]] virtual uint32_t GetCPUCoreCount() const = 0;
        [[nodiscard]] virtual uint64_t GetSystemMemoryBytes() const = 0;
    };
}