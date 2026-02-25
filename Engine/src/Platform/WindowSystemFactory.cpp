#include "npch.h"
#include "Nalta/Platform/WindowSystemFactory.h"

#ifdef N_PLATFORM_WINDOWS
#include "Nalta/Platform/Windows/WindowsWindowSystem.h"
#elif N_PLATFORM_LINUX
#include "Nalta/Platform/Linux/LinuxWindowSystem.h"
#elif N_PLATFORM_MACOS
#include "Nalta/Platform/Mac/MacWindowSystem.h"
#endif

namespace Nalta
{
    std::unique_ptr<IWindowSystem> CreateWindowSystem()
    {
#ifdef N_PLATFORM_WINDOWS
        return std::make_unique<WindowsWindowSystem>();
#else
        static_assert(false, "Platform not supported");
#endif
    }
}
