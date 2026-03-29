#include "npch.h"
#include "Nalta/Assets/Processors/TextureProcessor.h"
#include "Nalta/Assets/Processors/RawAssetData.h"
#include "Nalta/Assets/Texture/TextureAsset.h"
#include "Nalta/Graphics/GraphicsSystem.h"
#include "Nalta/Graphics/Texture/TextureDesc.h"

namespace Nalta
{
    bool TextureProcessor::Process(const RawAssetData& aBaseData, Asset& aOutAsset, GraphicsSystem& aGraphicsSystem) const
    {
        NL_SCOPE_CORE("TextureProcessor");

        const auto& data{ static_cast<const RawTextureData&>(aBaseData) };
        auto& texture{ static_cast<TextureAsset&>(aOutAsset) };

        if (!data.IsValid())
        {
            NL_ERROR(GCoreLogger, "invalid raw texture data");
            return false;
        }

        Graphics::TextureDesc desc;
        desc.width     = data.width;
        desc.height    = data.height;
        desc.mipLevels = data.mipLevels;
        desc.format    = data.format;
        desc.mips.reserve(data.mips.size());

        std::vector<uint8_t> pixels;
        for (const auto& mip : data.mips)
        {
            desc.mips.push_back({ mip.rowPitch, mip.slicePitch });
            pixels.insert(pixels.end(), mip.pixels.begin(), mip.pixels.end());
        }

        texture.myTextureHandle = aGraphicsSystem.CreateTexture(desc, std::as_bytes(std::span(pixels)));

        if (!texture.myTextureHandle.IsValid())
        {
            NL_ERROR(GCoreLogger, "failed to create GPU texture");
            return false;
        }

        NL_INFO(GCoreLogger, "processed texture ({}x{}, {} mips)", data.width, data.height, data.mipLevels);
        return true;
    }
}