#include "npch.h"
#include "Nalta/Assets/Processors/TextureProcessor.h"
#include "Nalta/Assets/RawAssetData.h"
#include "Nalta/Assets/Texture.h"
#include "Nalta/Graphics/GPUResourceManager.h"
#include "Nalta/RHI/Types/RHIDescs.h"

namespace Nalta
{
    bool TextureProcessor::Process(const RawTextureData& aRawData, Texture& outTexture, Graphics::GPUResourceManager& aGpuResources)
    {
        NL_SCOPE_CORE("TextureProcessor");

        if (!aRawData.IsValid())
        {
            NL_ERROR(GCoreLogger, "invalid raw texture data");
            return false;
        }

        RHI::TextureCreationDesc desc{};
        desc.width     = aRawData.width;
        desc.height    = aRawData.height;
        desc.mipLevels = static_cast<uint16_t>(aRawData.mipLevels);
        desc.format    = aRawData.textureFormat;
        desc.viewFlags = RHI::TextureViewFlags::ShaderResource;
        desc.debugName = aRawData.sourcePath.IsEmpty() ? "cooked texture" : aRawData.sourcePath.GetPath();

        RHI::TextureUploadDesc upload{};
        upload.desc = desc;
        upload.mips.reserve(aRawData.mips.size());

        for (const auto& mip : aRawData.mips)
        {
            RHI::TextureMipData mipData{};
            mipData.rowPitch = mip.rowPitch;
            mipData.slicePitch = mip.slicePitch;
            mipData.data = std::as_bytes(std::span{ mip.pixels });
            upload.mips.push_back(mipData);
        }

        outTexture.gpuTexture = aGpuResources.UploadTexture(upload);
        if (!outTexture.gpuTexture.IsValid())
        {
            NL_ERROR(GCoreLogger, "failed to upload texture");
            return false;
        }

        outTexture.width = aRawData.width;
        outTexture.height = aRawData.height;
        outTexture.mipLevels = aRawData.mipLevels;

        NL_INFO(GCoreLogger, "processed texture ({}x{}, {} mips)", aRawData.width, aRawData.height, aRawData.mipLevels);
        return true;
    }
}
