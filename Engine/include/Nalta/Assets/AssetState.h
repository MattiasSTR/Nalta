#pragma once
#include <cstdint>

namespace Nalta
{
    enum class AssetState : uint8_t
    {
        Unloaded,
        Requested,
        Loading,
        Processing,
        Uploading,
        Ready,
        Failed
    };
}