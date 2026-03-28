#pragma once
#include <cstdint>

namespace Nalta
{
    enum class AssetType : uint8_t
    {
        Unknown,
        Mesh,
        Texture,
        Material,
    };
}