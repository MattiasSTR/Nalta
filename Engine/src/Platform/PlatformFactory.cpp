#include "npch.h"
#include "Nalta/Platform/PlatformFactory.h"

#ifdef N_PLATFORM_WINDOWS
#include "Nalta/Platform/Win32/Win32PlatformSystem.h"
#include "Nalta/Platform/Win32/Win32FileWatcher.h"
#elif N_PLATFORM_LINUX
#include "Nalta/Platform/Linux/LinuxWindowSystem.h"
#elif N_PLATFORM_MACOS
#include "Nalta/Platform/Mac/MacWindowSystem.h"
#endif

namespace Nalta::PlatformFactory
{
    std::unique_ptr<IPlatformSystem> CreatePlatformSystem()
    {
#ifdef N_PLATFORM_WINDOWS
        return std::make_unique<Win32PlatformSystem>();
#else
        static_assert(false, "Platform not supported");
#endif
    }

    std::unique_ptr<IFileWatcher> CreateFileWatcher()
    {
#ifdef N_PLATFORM_WINDOWS
        return std::make_unique<Win32FileWatcher>();
#else
        static_assert(false, "Platform not supported");
#endif
    }
}
