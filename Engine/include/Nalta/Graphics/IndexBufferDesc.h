#pragma once
#include "IIndexBuffer.h"
#include <cstdint>

namespace Nalta::Graphics
{
    struct IndexBufferDesc
    {
        uint32_t count{ 0 };
        IndexFormat format{ IndexFormat::Uint32 };
    };
}