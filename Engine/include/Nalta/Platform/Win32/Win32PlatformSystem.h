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
        struct Impl;
        
        Win32PlatformSystem();
        ~Win32PlatformSystem() override;
        
        void Initialize() override;
        void Shutdown() override;
        bool PollEvents() override;
        void TickInput() override;
        
        [[nodiscard]] WindowHandle CreatePlatformWindow(const WindowDesc& aDesc) override;
        void DestroyWindow(WindowHandle aHandle) override;
        
        [[nodiscard]] WindowHandle GetMainWindow() const override;
        [[nodiscard]] InputSystem& GetInputSystem() override;
        
        void SetOnWindowDestroyedCallback(OnWindowDestroyedCallback aCallback) override;
        
        void SetCurrentThreadName(const std::string& aName) const override;

        [[nodiscard]] uint32_t GetCPUCoreCount() const override;
        [[nodiscard]] uint64_t GetSystemMemoryBytes() const override;
        
    private:
        std::unique_ptr<Impl> myImpl;

        std::vector<std::unique_ptr<Win32Window>> myWindows;
    };
}
