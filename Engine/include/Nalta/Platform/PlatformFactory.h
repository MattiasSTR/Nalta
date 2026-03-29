#pragma once
#include "IFileWatcher.h"
#include "IPlatformSystem.h"

#include <memory>

namespace Nalta
{
    std::unique_ptr<IPlatformSystem> CreatePlatformSystem();
    std::unique_ptr<IFileWatcher> CreateFileWatcher();
}
