#pragma once
#include "Nalta/Assets/AssetState.h"
#include "Nalta/Graphics/GPUResourceKeys.h"

#include <cstdint>

namespace Nalta
{
    struct Texture
    {
        Graphics::TextureKey gpuTexture{};
        uint32_t textureIndex{ ~0u };
        
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        uint32_t mipLevels{ 0 };
        AssetState state{ AssetState::Unloaded };
    };
}