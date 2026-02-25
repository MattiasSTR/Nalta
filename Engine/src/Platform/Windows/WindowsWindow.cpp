#include "npch.h"
#include "Nalta/Platform/Windows/WindowsWindow.h"

namespace Nalta
{
    WindowsWindow::WindowsWindow(const WindowDesc& aDesc)
    {
        aDesc;
    }

    WindowsWindow::~WindowsWindow()
    {
    }

    bool WindowsWindow::IsClosed() const
    {
        return false;
    }

    uint32_t WindowsWindow::GetWidth() const
    {
        return 0;
    }

    uint32_t WindowsWindow::GetHeight() const
    {
        return 0;
    }

    void WindowsWindow::SetSize(uint32_t aWidth, uint32_t aHeight)
    {
        aWidth;
        aHeight;
    }

    void WindowsWindow::SetFullscreen(bool aFullscreen)
    {
        aFullscreen;
    }

    bool WindowsWindow::IsFullscreen() const
    {
        return false;
    }

    void WindowsWindow::SetCaption(const std::string& aCaption)
    {
        aCaption;
    }

    void* WindowsWindow::GetNativeHandle() const
    {
        return nullptr;
    }
}
