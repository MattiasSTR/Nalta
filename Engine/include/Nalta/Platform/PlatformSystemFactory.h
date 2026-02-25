#pragma once
#include "IPlatformSystem.h"

#include <memory>

namespace Nalta
{
    std::unique_ptr<IPlatformSystem> CreateWindowSystem();
}
