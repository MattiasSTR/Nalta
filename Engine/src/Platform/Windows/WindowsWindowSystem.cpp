#include "npch.h"
#include "Nalta/Platform/Windows/WindowsWindowSystem.h"

namespace Nalta
{
    WindowsWindowSystem::WindowsWindowSystem()
    {
    }

    WindowsWindowSystem::~WindowsWindowSystem()
    {
    }

    void WindowsWindowSystem::Initialize()
    {
    }

    void WindowsWindowSystem::Shutdown()
    {
    }

    void WindowsWindowSystem::PollEvents()
    {
    }

    std::shared_ptr<IWindow> WindowsWindowSystem::CreateWindow(const WindowDesc& aDesc)
    {
        aDesc;
        return {};
    }
}
