#pragma once
#include "Nalta/Core/Window/IWindowSystem.h"

#include <memory>
#include <vector>

namespace Nalta
{
    class WindowsWindowSystem : public IWindowSystem
    {
    public:
        WindowsWindowSystem();
        ~WindowsWindowSystem() override;
        
        void Initialize() override;
        void Shutdown() override;
        
        void PollEvents() override;
        
        std::shared_ptr<IWindow> CreateWindow(const WindowDesc& aDesc) override;
        
    private:
        std::vector<std::shared_ptr<IWindow>> myWindows;
    };
}
