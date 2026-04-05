#pragma once
#include "Nalta/Platform/IPlatformSystem.h"
#include "Nalta/Util/SlotMap.h"

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
        
        [[nodiscard]] WindowKey CreatePlatformWindow(const WindowDesc& aDesc) override;
        [[nodiscard]] IWindow* GetWindow(WindowKey aKey) override;
        void DestroyWindow(WindowKey aKey) override;
        
        [[nodiscard]] InputSystem& GetInputSystem() override;
        
        void SetOnWindowDestroyedCallback(OnWindowDestroyedCallback aCallback) override;
        
        void SetCurrentThreadName(const std::string& aName) const override;

        [[nodiscard]] uint32_t GetCPUCoreCount() const override;
        [[nodiscard]] uint64_t GetSystemMemoryBytes() const override;
        
    private:

        std::unique_ptr<Impl> myImpl;

        SlotMap<WindowKey, std::unique_ptr<Win32Window>> myWindows;
    };
}
