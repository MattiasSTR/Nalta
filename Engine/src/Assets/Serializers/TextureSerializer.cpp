#include "npch.h"
#include "Nalta/Assets/Serializers/TextureSerializer.h"
#include "Nalta/Assets/Processors/RawAssetData.h"
#include "Nalta/Core/BinaryIO.h"

namespace Nalta
{
    void TextureSerializer::Write(const RawAssetData& aBaseData, BinaryWriter& aWriter) const
    {
        NL_SCOPE_CORE("TextureSerializer");
        const auto& data{ static_cast<const RawTextureData&>(aBaseData) };

        aWriter.Write(data.width);
        aWriter.Write(data.height);
        aWriter.Write(data.mipLevels);
        aWriter.Write(static_cast<uint8_t>(data.format));

        aWriter.Write(static_cast<uint32_t>(data.mips.size()));
        for (const auto& mip : data.mips)
        {
            aWriter.Write(mip.rowPitch);
            aWriter.Write(mip.slicePitch);
            aWriter.Write(static_cast<uint32_t>(mip.pixels.size()));
            aWriter.WriteBytes(std::span(mip.pixels));
        }

        NL_TRACE(GCoreLogger, "wrote {}x{} {} mips", data.width, data.height, data.mips.size());
    }

    std::unique_ptr<RawAssetData> TextureSerializer::Read(BinaryReader& aReader) const
    {
        NL_SCOPE_CORE("TextureSerializer");
        auto data{ std::make_unique<RawTextureData>() };

        data->width     = aReader.Read<uint32_t>();
        data->height    = aReader.Read<uint32_t>();
        data->mipLevels = aReader.Read<uint32_t>();
        data->format    = static_cast<Graphics::TextureFormat>(aReader.Read<uint8_t>());

        const uint32_t mipCount{ aReader.Read<uint32_t>() };
        data->mips.resize(mipCount);
        for (auto& mip : data->mips)
        {
            mip.rowPitch   = aReader.Read<uint32_t>();
            mip.slicePitch = aReader.Read<uint32_t>();
            const uint32_t pixelSize{ aReader.Read<uint32_t>() };
            mip.pixels.resize(pixelSize);
            aReader.ReadBytes(std::span(mip.pixels));
        }

        NL_TRACE(GCoreLogger, "read {}x{} {} mips", data->width, data->height, data->mips.size());
        return data;
    }
}