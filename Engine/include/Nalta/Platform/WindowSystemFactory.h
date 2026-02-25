#pragma once
#include "Nalta/Core/Window/IWindowSystem.h"

#include <memory>

namespace Nalta
{
    std::unique_ptr<IWindowSystem> CreateWindowSystem();
}
