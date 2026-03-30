#include "npch.h"
#include "Nalta/Assets/Importers/DDSImporter.h"
#include "Nalta/Assets/RawAssetData.h"
#include "Nalta/Graphics/Texture/TextureDesc.h"

#include <DirectXTex.h>

namespace Nalta
{
    namespace
    {
        Graphics::TextureFormat FromDXGIFormat(const DXGI_FORMAT aFormat)
        {
            switch (aFormat)
            {
                case DXGI_FORMAT_R8G8B8A8_UNORM:      return Graphics::TextureFormat::RGBA8_UNORM;
                case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return Graphics::TextureFormat::RGBA8_SRGB;
                case DXGI_FORMAT_BC1_UNORM:            return Graphics::TextureFormat::BC1_UNORM;
                case DXGI_FORMAT_BC1_UNORM_SRGB:       return Graphics::TextureFormat::BC1_SRGB;
                case DXGI_FORMAT_BC3_UNORM:            return Graphics::TextureFormat::BC3_UNORM;
                case DXGI_FORMAT_BC3_UNORM_SRGB:       return Graphics::TextureFormat::BC3_SRGB;
                case DXGI_FORMAT_BC4_UNORM:            return Graphics::TextureFormat::BC4_UNORM;
                case DXGI_FORMAT_BC5_UNORM:            return Graphics::TextureFormat::BC5_UNORM;
                case DXGI_FORMAT_BC7_UNORM:            return Graphics::TextureFormat::BC7_UNORM;
                case DXGI_FORMAT_BC7_UNORM_SRGB:       return Graphics::TextureFormat::BC7_SRGB;
                case DXGI_FORMAT_BC6H_UF16:            return Graphics::TextureFormat::BC6H_UF16;
                default:                               return Graphics::TextureFormat::RGBA8_UNORM;
            }
        }
    }

    bool DDSImporter::CanImport(const std::string& aExtension) const
    {
        return aExtension == ".dds";
    }

    std::unique_ptr<RawAssetData> DDSImporter::Import(const AssetPath& aPath) const
    {
        NL_SCOPE_CORE("DDSImporter");
        NL_TRACE(GCoreLogger, "importing '{}'", aPath.GetPath());

        DirectX::ScratchImage image;
        const std::wstring widePath{ std::filesystem::path(aPath.GetPath()).wstring() };

        if (FAILED(DirectX::LoadFromDDSFile(widePath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image)))
        {
            NL_ERROR(GCoreLogger, "failed to load '{}'", aPath.GetPath());
            return nullptr;
        }

        const auto& meta{ image.GetMetadata() };
        auto result{ std::make_unique<RawTextureData>() };
        result->sourcePath = aPath;
        result->width      = static_cast<uint32_t>(meta.width);
        result->height     = static_cast<uint32_t>(meta.height);
        result->mipLevels  = static_cast<uint32_t>(meta.mipLevels);
        result->format     = FromDXGIFormat(meta.format);

        for (size_t mip{ 0 }; mip < meta.mipLevels; ++mip)
        {
            const auto* img{ image.GetImage(mip, 0, 0) };
            if (img == nullptr)
            {
                continue;
            }

            RawTextureMip mipData;
            mipData.rowPitch   = static_cast<uint32_t>(img->rowPitch);
            mipData.slicePitch = static_cast<uint32_t>(img->slicePitch);
            mipData.pixels.assign(img->pixels, img->pixels + img->slicePitch);
            result->mips.push_back(std::move(mipData));
        }

        NL_INFO(GCoreLogger, "imported '{}' ({}x{}, {} mips, format={})",
            aPath.GetPath(), result->width, result->height, result->mipLevels,
            static_cast<uint32_t>(meta.format));
        return result;
    }
}