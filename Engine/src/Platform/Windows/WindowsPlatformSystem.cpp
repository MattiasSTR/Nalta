#include "npch.h"
#include "Nalta/Platform/Windows/WindowsPlatformSystem.h"

#include "Nalta/Platform/Windows/WindowsWindow.h"

#pragma warning(push, 0)
#include <GLFW/glfw3.h>
#pragma warning(pop)

namespace Nalta
{
    namespace
    {
#ifndef N_SHIPPING
        void GLFWErrorCallback(int aError, const char* aDescription)
        {
            NL_ERROR(GCoreLogger, "GLFW Error ({}): {}", aError, aDescription);
        }
#endif
    }
    
    WindowsPlatformSystem::WindowsPlatformSystem() = default;
    WindowsPlatformSystem::~WindowsPlatformSystem() = default;

    void WindowsPlatformSystem::Initialize()
    {
#ifndef N_SHIPPING
        glfwSetErrorCallback(GLFWErrorCallback);
#endif
        if (glfwInit() == 0)
        {
            NL_FATAL(GCoreLogger, "GLFW Initialization failed!");
        }
        myInitialized = true;

        int major{};
        int minor{};
        int revision{};
        glfwGetVersion(&major, &minor, &revision);
        NL_INFO(GCoreLogger, "GLFW Version: {}.{}.{}", major, minor, revision);
        NL_INFO(GCoreLogger, "GLFW initialized for WindowsWindowSystem");
    }

    void WindowsPlatformSystem::Shutdown()
    {
        if (myInitialized)
        {
            glfwTerminate();
            myInitialized = false;
            NL_INFO(GCoreLogger, "GLFW terminated for WindowsWindowSystem");
        }
    }

    void WindowsPlatformSystem::PollEvents()
    {
        glfwPollEvents();
    }

    std::shared_ptr<IWindow> WindowsPlatformSystem::CreateWindow(const WindowDesc& aDesc)
    {
        return std::make_shared<WindowsWindow>(aDesc);
    }
}
