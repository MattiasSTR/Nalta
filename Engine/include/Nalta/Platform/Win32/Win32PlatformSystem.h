#pragma once
#include "Nalta/Platform/IPlatformSystem.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Nalta
{
    class Win32Window;
    
    class Win32PlatformSystem : public IPlatformSystem
    {
    public:
        Win32PlatformSystem();
        ~Win32PlatformSystem() override;
        
        void Initialize() override;
        void Shutdown() override;
        bool PollEvents() override;
        
        [[nodiscard]] WindowHandle CreatePlatformWindow(const WindowDesc& aDesc) override;
        void DestroyWindow(WindowHandle aHandle) override;
        
        [[nodiscard]] WindowHandle GetMainWindow() const override;
        
    private:
        struct Impl;
        std::unique_ptr<Impl> myImpl;

        std::vector<std::unique_ptr<Win32Window>> myWindows;
    };
}
