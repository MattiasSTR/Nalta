#include "npch.h"
#include "Nalta/Assets/Processors/TextureProcessor.h"
#include "Nalta/Assets/RawAssetData.h"
#include "Nalta/Assets/Texture.h"
#include "Nalta/Graphics/GraphicsSystem.h"
#include "Nalta/Graphics/Texture/TextureDesc.h"

namespace Nalta
{
    bool TextureProcessor::Process(const RawTextureData& aRawData, Texture& outTexture, GraphicsSystem& aGraphicsSystem)
    {
        NL_SCOPE_CORE("TextureProcessor");

        if (!aRawData.IsValid())
        {
            NL_ERROR(GCoreLogger, "invalid raw texture data");
            return false;
        }

        Graphics::TextureDesc desc;
        desc.width = aRawData.width;
        desc.height = aRawData.height;
        desc.mipLevels = aRawData.mipLevels;
        desc.format = aRawData.format;
        desc.mips.reserve(aRawData.mips.size());

        std::vector<uint8_t> pixels;
        for (const auto& mip : aRawData.mips)
        {
            desc.mips.push_back({ mip.rowPitch, mip.slicePitch });
            pixels.insert(pixels.end(), mip.pixels.begin(), mip.pixels.end());
        }

        outTexture.gpuHandle = aGraphicsSystem.CreateTexture(desc, std::as_bytes(std::span(pixels)));

        if (!outTexture.gpuHandle.IsValid())
        {
            NL_ERROR(GCoreLogger, "failed to create GPU texture");
            return false;
        }

        NL_INFO(GCoreLogger, "processed texture ({}x{}, {} mips)", aRawData.width, aRawData.height, aRawData.mipLevels);
        return true;
    }
}
