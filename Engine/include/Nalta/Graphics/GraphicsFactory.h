#pragma once
#include "IDevice.h"

namespace Nalta::Graphics
{
    std::unique_ptr<IDevice> CreateDevice();
}
