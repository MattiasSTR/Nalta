#pragma once
#include "Nalta/Platform/IPlatformSystem.h"

#include <memory>

namespace Nalta
{
    class WindowsPlatformSystem : public IPlatformSystem
    {
    public:
        WindowsPlatformSystem();
        ~WindowsPlatformSystem() override;
        
        void Initialize() override;
        void Shutdown() override;
        
        void PollEvents() override;
        
        std::shared_ptr<IWindow> CreateWindow(const WindowDesc& aDesc) override;
        
    private:
        bool myInitialized{ false };
    };
}
