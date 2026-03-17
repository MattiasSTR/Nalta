#include "npch.h"
#include "Nalta/Platform/PlatformSystemFactory.h"

#ifdef N_PLATFORM_WINDOWS
#include "Nalta/Platform/Win32/Win32PlatformSystem.h"
#elif N_PLATFORM_LINUX
#include "Nalta/Platform/Linux/LinuxWindowSystem.h"
#elif N_PLATFORM_MACOS
#include "Nalta/Platform/Mac/MacWindowSystem.h"
#endif

namespace Nalta
{
    std::unique_ptr<IPlatformSystem> CreateWindowSystem()
    {
#ifdef N_PLATFORM_WINDOWS
        return std::make_unique<Win32PlatformSystem>();
#else
        static_assert(false, "Platform not supported");
#endif
    }
}
