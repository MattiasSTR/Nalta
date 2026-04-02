#include "npch.h"
#include "Nalta/Assets/Importers/TextureDescriptorImporter.h"
#include "Nalta/Assets/Importers/TextureImporter.h"
#include "Nalta/Assets/RawAssetData.h"
//#include "Nalta/Graphics/Texture/TextureDesc.h"

#include <dxgiformat.h>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Nalta
{
    // namespace
    // {
    //     Graphics::TextureFormat ParseCompression(const std::string& aStr, const bool aSRGB)
    //     {
    //         if (aStr == "BC1")   return aSRGB ? Graphics::TextureFormat::BC1_SRGB  : Graphics::TextureFormat::BC1_UNORM;
    //         if (aStr == "BC3")   return aSRGB ? Graphics::TextureFormat::BC3_SRGB  : Graphics::TextureFormat::BC3_UNORM;
    //         if (aStr == "BC4")   return Graphics::TextureFormat::BC4_UNORM;
    //         if (aStr == "BC5")   return Graphics::TextureFormat::BC5_UNORM;
    //         if (aStr == "BC7")   return aSRGB ? Graphics::TextureFormat::BC7_SRGB  : Graphics::TextureFormat::BC7_UNORM;
    //         if (aStr == "BC6H")  return Graphics::TextureFormat::BC6H_UF16;
    //         if (aStr == "RGBA8") return aSRGB ? Graphics::TextureFormat::RGBA8_SRGB : Graphics::TextureFormat::RGBA8_UNORM;
    //         NL_WARN(GCoreLogger, "TextureDescriptorImporter: unknown compression '{}', defaulting to BC1", aStr);
    //         return Graphics::TextureFormat::BC1_UNORM;
    //     }
    // }
    
    bool TextureDescriptorImporter::CanImport(const std::string& aExtension) const
    {
        return aExtension == ".texture";
    }
    
    std::unique_ptr<RawAssetData> TextureDescriptorImporter::Import(const AssetPath& aPath) const
    {
        NL_SCOPE_CORE("TextureDescriptorImporter");
        NL_TRACE(GCoreLogger, "importing '{}'", aPath.GetPath());

        std::ifstream file{ aPath.GetPath() };
        if (!file.is_open())
        {
            NL_ERROR(GCoreLogger, "failed to open '{}'", aPath.GetPath());
            return nullptr;
        }

        json root;
        try { root = json::parse(file); }
        catch (const json::exception& e)
        {
            NL_ERROR(GCoreLogger, "JSON parse error in '{}': {}", aPath.GetPath(), e.what());
            return nullptr;
        }

        const std::string sourcePath{ root.value("source", "") };
        const std::string compression{ root.value("compression", "BC1") };
        const bool sRGB{ root.value("sRGB", false) };
        const bool generateMips{ root.value("generateMips", true) };

        if (sourcePath.empty())
        {
            NL_ERROR(GCoreLogger, "no source specified in '{}'", aPath.GetPath());
            return nullptr;
        }

        // Resolve source path relative to descriptor file
        const auto descriptorDir{ std::filesystem::path(aPath.GetPath()).parent_path() };
        const auto resolvedSource{ std::filesystem::weakly_canonical(descriptorDir / sourcePath) };
        std::string normalizedSource{ resolvedSource.string() };
        std::ranges::replace(normalizedSource, '\\', '/');

        //const Graphics::TextureFormat targetFormat{ ParseCompression(compression, sRGB) };

        // Delegate actual loading to TextureImporter
        const AssetPath sourceAssetPath{ normalizedSource };
        TextureImporter importer{ sourceAssetPath.GetExtension() };
        auto rawData{ importer.ImportWithSettings(sourceAssetPath, {}/*targetFormat*/, generateMips) };

        if (!rawData)
        {
            NL_ERROR(GCoreLogger, "failed to import source '{}' for '{}'", normalizedSource, aPath.GetPath());
            return nullptr;
        }
        
        auto& texData{ static_cast<RawTextureData&>(*rawData) };
        texData.sourceImagePath = normalizedSource;

        NL_INFO(GCoreLogger, "imported descriptor '{}' -> '{}' ({}, {})", aPath.GetPath(), normalizedSource, compression, sRGB ? "sRGB" : "linear");
        return rawData;
    }
}