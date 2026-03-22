#pragma once
#include <cstdint>

namespace Nalta::Graphics
{
    struct VertexBufferDesc
    {
        uint32_t stride{ 0 };
        uint32_t count{ 0 };
    };
}