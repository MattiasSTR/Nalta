#pragma once
#include "Nalta/Assets/AssetState.h"
//#include "Nalta/Graphics/Texture/TextureHandle.h"

#include <cstdint>

namespace Nalta
{
    struct Texture
    {
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        uint32_t mipLevels{ 0 };
        //Graphics::TextureHandle gpuHandle;
        AssetState state{ AssetState::Unloaded };
    };
}