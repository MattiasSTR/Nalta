#pragma once
#include <cstdint>

namespace Nalta::Graphics
{
    struct DeviceDesc
    {
        uint32_t framesInFlight{ 2 };
    };
}