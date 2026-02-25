#include "npch.h"
#include "Nalta/Platform/Windows/WindowsWindow.h"

#include "Nalta/Platform/WindowDesc.h"

#pragma warning(push, 0)
#include <GLFW/glfw3.h>
#pragma warning(pop)

namespace Nalta
{
    WindowsWindow::WindowsWindow(const WindowDesc& aDesc)
    {
        glfwWindowHint(GLFW_RESIZABLE, aDesc.resizable ? GLFW_TRUE : GLFW_FALSE);
        
        myNativeWindow = glfwCreateWindow(
            static_cast<int32_t>(aDesc.width),
            static_cast<int32_t>(aDesc.height),
            aDesc.caption.c_str(),
            myFullscreen ? glfwGetPrimaryMonitor() : nullptr,
            nullptr
        );
        
        if (myNativeWindow == nullptr)
        {
            NL_FATAL(GCoreLogger, "Failed to create Windows Window!");
        }
        
        NL_INFO(GCoreLogger, "GLFW window created successfully");
    }

    WindowsWindow::~WindowsWindow()
    {
        if (myNativeWindow != nullptr)
        {
            glfwDestroyWindow(myNativeWindow);
            myNativeWindow = nullptr;
            NL_INFO(GCoreLogger, "GLFW window destroyed");
        }
    }

    bool WindowsWindow::IsClosed() const
    {
        return (myNativeWindow != nullptr) ? glfwWindowShouldClose(myNativeWindow) != 0 : true;
    }

    uint32_t WindowsWindow::GetWidth() const
    {
        if (myNativeWindow != nullptr)
        {
            int w{ 0 };
            int h{ 0 };
            glfwGetWindowSize(myNativeWindow, &w, &h);
            return static_cast<uint32_t>(w);
        }
        return myWidth;
    }

    uint32_t WindowsWindow::GetHeight() const
    {
        if (myNativeWindow != nullptr)
        {
            int w{ 0 };
            int h{ 0 };
            glfwGetWindowSize(myNativeWindow, &w, &h);
            return static_cast<uint32_t>(h);
        }
        return myHeight;
    }

    void WindowsWindow::Resize(const uint32_t aWidth, const uint32_t aHeight)
    {
        if (myNativeWindow != nullptr)
        {
            glfwSetWindowSize(myNativeWindow, static_cast<int>(aWidth), static_cast<int>(aHeight));
            myWidth = aWidth;
            myHeight = aHeight;
        }
    }

    void WindowsWindow::SetFullscreen(const bool aFullscreen)
    {
        if (myNativeWindow == nullptr)
        {
            return;
        }

        if (aFullscreen == myFullscreen)
        {
            return;
        }
        
        GLFWmonitor* monitor{ aFullscreen ? glfwGetPrimaryMonitor() : nullptr };
        glfwSetWindowMonitor(
            myNativeWindow, 
            monitor, 
            0, 0, 
            static_cast<int>(myWidth), 
            static_cast<int>(myHeight), 
            GLFW_DONT_CARE
        );
        
        myFullscreen = aFullscreen;
    }

    bool WindowsWindow::IsFullscreen() const
    {
        return myFullscreen;
    }

    void WindowsWindow::SetCaption(const std::string& aCaption)
    {
        if (myNativeWindow != nullptr)
        {
            glfwSetWindowTitle(myNativeWindow, aCaption.c_str());
        }
    }

    void* WindowsWindow::GetNativeHandle() const
    {
        return myNativeWindow;
    }
}
