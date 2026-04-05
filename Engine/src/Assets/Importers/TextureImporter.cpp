#include "npch.h"
#include "Nalta/Assets/Importers/TextureImporter.h"
#include "Nalta/Assets/RawAssetData.h"

#include <DirectXTex.h>

namespace Nalta
{
    namespace
    {
        RHI::TextureFormat FromDXGIFormat(const DXGI_FORMAT aFormat)
        {
            switch (aFormat)
            {
                case DXGI_FORMAT_R8G8B8A8_UNORM:       return RHI::TextureFormat::RGBA8_UNORM;
                case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:  return RHI::TextureFormat::RGBA8_SRGB;
                case DXGI_FORMAT_BC1_UNORM:            return RHI::TextureFormat::BC1_UNORM;
                case DXGI_FORMAT_BC1_UNORM_SRGB:       return RHI::TextureFormat::BC1_SRGB;
                case DXGI_FORMAT_BC3_UNORM:            return RHI::TextureFormat::BC3_UNORM;
                case DXGI_FORMAT_BC3_UNORM_SRGB:       return RHI::TextureFormat::BC3_SRGB;
                case DXGI_FORMAT_BC4_UNORM:            return RHI::TextureFormat::BC4_UNORM;
                case DXGI_FORMAT_BC5_UNORM:            return RHI::TextureFormat::BC5_UNORM;
                case DXGI_FORMAT_BC7_UNORM:            return RHI::TextureFormat::BC7_UNORM;
                case DXGI_FORMAT_BC7_UNORM_SRGB:       return RHI::TextureFormat::BC7_SRGB;
                case DXGI_FORMAT_BC6H_UF16:            return RHI::TextureFormat::BC6H_UF16;
                default:                               return RHI::TextureFormat::RGBA8_UNORM;
            }
        }
        
        DXGI_FORMAT ToCompressDXGI(const RHI::TextureFormat aFormat)
        {
            switch (aFormat)
            {
                case RHI::TextureFormat::BC1_UNORM:   return DXGI_FORMAT_BC1_UNORM;
                case RHI::TextureFormat::BC1_SRGB:    return DXGI_FORMAT_BC1_UNORM_SRGB;
                case RHI::TextureFormat::BC3_UNORM:   return DXGI_FORMAT_BC3_UNORM;
                case RHI::TextureFormat::BC3_SRGB:    return DXGI_FORMAT_BC3_UNORM_SRGB;
                case RHI::TextureFormat::BC4_UNORM:   return DXGI_FORMAT_BC4_UNORM;
                case RHI::TextureFormat::BC5_UNORM:   return DXGI_FORMAT_BC5_UNORM;
                case RHI::TextureFormat::BC7_UNORM:   return DXGI_FORMAT_BC7_UNORM;
                case RHI::TextureFormat::BC7_SRGB:    return DXGI_FORMAT_BC7_UNORM_SRGB;
                case RHI::TextureFormat::BC6H_UF16:   return DXGI_FORMAT_BC6H_UF16;
                case RHI::TextureFormat::RGBA8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
                case RHI::TextureFormat::RGBA8_SRGB:  return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                default:                                   return DXGI_FORMAT_BC1_UNORM;
            }
        }
        
        bool IsUncompressed(const RHI::TextureFormat aFormat)
        {
            return aFormat == RHI::TextureFormat::RGBA8_UNORM || aFormat == RHI::TextureFormat::RGBA8_SRGB;
        }
        
        bool IsHDR(const RHI::TextureFormat aFormat)
        {
            return aFormat == RHI::TextureFormat::BC6H_UF16;
        }
        
        struct COMInitializer
        {
            COMInitializer()  { CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
            ~COMInitializer() { CoUninitialize(); }
        };

        void EnsureCOMInitialized()
        {
            const thread_local COMInitializer comInit;
        }
    }
    
    TextureImporter::TextureImporter(std::string aExtension)
        : myExtension(std::move(aExtension))
    {}
    
    bool TextureImporter::CanImport(const std::string& aExtension) const
    {
        return aExtension == myExtension;
    }

    std::string TextureImporter::GetName() const
    {
        return "TextureImporter(" + myExtension + ")";
    }

    std::unique_ptr<RawAssetData> TextureImporter::Import(const AssetPath& aPath) const
    {
        return ImportWithSettings(aPath, RHI::TextureFormat::BC1_UNORM, true);
    }
    
    std::unique_ptr<RawAssetData> TextureImporter::ImportWithSettings(const AssetPath& aPath, RHI::TextureFormat aTargetFormat, bool aGenerateMips) const
    {
        EnsureCOMInitialized();
        NL_SCOPE_CORE("TextureImporter");
        NL_TRACE(GCoreLogger, "importing '{}'", aPath.GetPath());

        DirectX::ScratchImage image;
        const std::wstring widePath{ std::filesystem::path(aPath.GetPath()).wstring() };

        HRESULT hr{ E_FAIL };
        if (myExtension == ".tga")
        {
            hr = DirectX::LoadFromTGAFile(widePath.c_str(), nullptr, image);
        }
        else
        {
            hr = DirectX::LoadFromWICFile(widePath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);
        }
        
        if (FAILED(hr))
        {
            NL_ERROR(GCoreLogger, "failed to load '{}'", aPath.GetPath());
            return nullptr;
        }
        
        const DXGI_FORMAT convertTarget{ IsHDR(aTargetFormat) ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM };
        
        const auto& meta{ image.GetMetadata() };
        if (meta.format != convertTarget)
        {
            DirectX::ScratchImage converted;
            if (FAILED(DirectX::Convert(
                image.GetImages(), image.GetImageCount(), meta,
                convertTarget, DirectX::TEX_FILTER_DEFAULT,
                DirectX::TEX_THRESHOLD_DEFAULT, converted)))
            {
                NL_ERROR(GCoreLogger, "failed to convert '{}'", aPath.GetPath());
                return nullptr;
            }
            image = std::move(converted);
        }
        
        DirectX::ScratchImage mipped;
        if (aGenerateMips)
        {
            if (FAILED(DirectX::GenerateMipMaps(
                image.GetImages(), image.GetImageCount(), image.GetMetadata(),
                DirectX::TEX_FILTER_DEFAULT, 0, mipped)))
            {
                NL_WARN(GCoreLogger, "mip generation failed - using base only");
                mipped = std::move(image);
            }
        }
        else
        {
            mipped = std::move(image);
        }
        
        DirectX::TEX_COMPRESS_FLAGS compressFlags{ DirectX::TEX_COMPRESS_DEFAULT | DirectX::TEX_COMPRESS_PARALLEL };
 #ifndef N_SHIPPING
         if (ToCompressDXGI(aTargetFormat) == DXGI_FORMAT_BC7_UNORM || ToCompressDXGI(aTargetFormat) == DXGI_FORMAT_BC7_UNORM_SRGB)
         {
             compressFlags |= DirectX::TEX_COMPRESS_BC7_QUICK;
         }
 #endif
        
        DirectX::ScratchImage compressed;
        if (!IsUncompressed(aTargetFormat))
        {
            if (FAILED(DirectX::Compress(
                mipped.GetImages(), mipped.GetImageCount(), mipped.GetMetadata(),
                ToCompressDXGI(aTargetFormat),
                compressFlags,
                DirectX::TEX_THRESHOLD_DEFAULT, compressed)))
            {
                NL_WARN(GCoreLogger, "compression failed - falling back to RGBA8");
                compressed = std::move(mipped);
            }
        }
        else
        {
            compressed = std::move(mipped);
        }
        
        const auto& finalMeta{ compressed.GetMetadata() };
        auto result{ std::make_unique<RawTextureData>() };
        result->sourcePath = aPath;
        result->width      = static_cast<uint32_t>(finalMeta.width);
        result->height     = static_cast<uint32_t>(finalMeta.height);
        result->mipLevels  = static_cast<uint32_t>(finalMeta.mipLevels);
        result->textureFormat = FromDXGIFormat(finalMeta.format);

        for (size_t mip{ 0 }; mip < finalMeta.mipLevels; ++mip)
        {
            const auto* img{ compressed.GetImage(mip, 0, 0) };
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

        NL_INFO(GCoreLogger, "imported '{}' ({}x{}, {} mips)", aPath.GetPath(), result->width, result->height, result->mipLevels);
        return result;
    }
}