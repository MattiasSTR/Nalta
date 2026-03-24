#pragma once
#include <cstdint>

namespace Nalta::Graphics
{
    struct ConstantBufferDesc
    {
        uint32_t size{ 0 }; // Size of one constant buffer slot in bytes
    };
}