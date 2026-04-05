#include "npch.h"
#include "Nalta/Assets/Serializers/TextureSerializer.h"
#include "Nalta/Assets/RawAssetData.h"
#include "Nalta/Core/BinaryIO.h"

namespace Nalta
{
    void TextureSerializer::Write(const RawTextureData& aRawData, BinaryWriter& aWriter)
    {
        NL_SCOPE_CORE("TextureSerializer");

        aWriter.Write(aRawData.width);
        aWriter.Write(aRawData.height);
        aWriter.Write(aRawData.mipLevels);
        aWriter.Write(static_cast<uint8_t>(aRawData.textureFormat));

        aWriter.Write(static_cast<uint32_t>(aRawData.mips.size()));
        for (const auto& mip : aRawData.mips)
        {
            aWriter.Write(mip.rowPitch);
            aWriter.Write(mip.slicePitch);
            aWriter.Write(static_cast<uint32_t>(mip.pixels.size()));
            aWriter.WriteBytes(std::span(mip.pixels));
        }

        NL_TRACE(GCoreLogger, "wrote {}x{} {} mips", aRawData.width, aRawData.height, aRawData.mips.size());
    }

    RawTextureData TextureSerializer::Read(BinaryReader& aReader)
    {
        NL_SCOPE_CORE("TextureSerializer");
        RawTextureData data{};

        data.width = aReader.Read<uint32_t>();
        data.height = aReader.Read<uint32_t>();
        data.mipLevels = aReader.Read<uint32_t>();
        data.textureFormat = static_cast<RHI::TextureFormat>(aReader.Read<uint8_t>());

        const uint32_t mipCount{ aReader.Read<uint32_t>() };
        data.mips.resize(mipCount);
        for (auto& mip : data.mips)
        {
            mip.rowPitch = aReader.Read<uint32_t>();
            mip.slicePitch = aReader.Read<uint32_t>();
            const uint32_t pixelSize{ aReader.Read<uint32_t>() };
            mip.pixels.resize(pixelSize);
            aReader.ReadBytes(std::span(mip.pixels));
        }

        NL_TRACE(GCoreLogger, "read {}x{} {} mips", data.width, data.height, data.mips.size());
        return data;
    }
}