#pragma once
#include "WindowHandle.h"

namespace Nalta
{
    class IWindow;
    struct WindowDesc;

    class IPlatformSystem
    {
    public:
        virtual ~IPlatformSystem() = default;

        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual bool PollEvents() = 0; // Returns false if wants to close main window

        [[nodiscard]] virtual WindowHandle CreatePlatformWindow(const WindowDesc& aDesc) = 0;
        virtual void DestroyWindow(WindowHandle aWindow) = 0;
        
        [[nodiscard]] virtual WindowHandle GetMainWindow() const = 0;
    };
}