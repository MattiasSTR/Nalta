#pragma once
#include <cstdint>

namespace Nalta::Graphics
{
    enum class ShaderStage : uint8_t
    {
        Vertex,
        Pixel,
        Compute
    };
}