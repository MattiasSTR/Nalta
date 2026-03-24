#pragma once
#include "Device/IDevice.h"

namespace Nalta::Graphics
{
    std::unique_ptr<IDevice> CreateDevice();
}
