#include "npch.h"
#include "Nalta/Assets/TypeHandlers/TextureTypeHandler.h"

#include "Nalta/Assets/AssetManager.h"
#include "Nalta/Assets/RawAssetData.h"
#include "Nalta/Assets/Importers/IAssetImporter.h"
#include "Nalta/Assets/Processors/TextureProcessor.h"
#include "Nalta/Assets/Serializers/TextureSerializer.h"
#include "Nalta/Core/BinaryIO.h"
#include "Nalta/Graphics/GPUResourceManager.h"

namespace Nalta
{
    bool TextureTypeHandler::CookAndProcess(const uint64_t aHash, const AssetPath& aPath, AssetRegistry& aRegistry)
    {
        const IAssetImporter* importer{ myImporters.GetImporter(aPath) };
        if (importer == nullptr)
        {
            NL_ERROR(GCoreLogger, "no importer for '{}'", aPath.GetPath());
            return false;
        }

        const auto rawBase{ importer->Import(aPath) };
        if (!rawBase || !rawBase->IsValid())
        {
            NL_ERROR(GCoreLogger, "import failed for '{}'", aPath.GetPath());
            return false;
        }

        auto& raw{ static_cast<RawTextureData&>(*rawBase) };
        const TextureKey key{ myStore.GetKey(aHash) };

        bool processOk{ false };
        myStore.Modify(key, [&](Texture& aTex)
        {
            processOk = TextureProcessor::Process(raw, aTex, *myGPU);
        });

        if (!processOk)
        {
            NL_ERROR(GCoreLogger, "texture processing failed for '{}'", aPath.GetPath());
            return false;
        }

#ifndef N_SHIPPING
        std::vector<std::string> deps;
        if (!raw.sourceImagePath.empty())
        {
            deps.push_back(raw.sourceImagePath);
        }

        BinaryWriter writer;
        AssetManager::WriteCookedHeader(writer, AssetType::Texture);
        TextureSerializer::Write(raw, writer);

        const auto cookedPath{ AssetManager::GetCookedPath(aPath) };
        if (writer.SaveToFile(cookedPath))
        {
            AssetRegistryEntry entry;
            entry.sourcePath   = aPath.GetPath();
            entry.cookedFile   = cookedPath.filename().string();
            entry.assetType    = "Texture";
            entry.dependencies = deps;

            if (std::filesystem::exists(aPath.GetPath()))
            {
                entry.lastModified = std::filesystem::last_write_time(aPath.GetPath());
            }

            aRegistry.Register(entry);
            aRegistry.Save();
            NL_INFO(GCoreLogger, "cooked texture to '{}'", cookedPath.string());
        }
#endif

        NL_INFO(GCoreLogger, "loaded texture '{}'", aPath.GetPath());
        return true;
    }

    bool TextureTypeHandler::LoadFromCooked(uint64_t aHash, const AssetPath& aSourcePath, const std::filesystem::path& aCookedPath)
    {
        auto reader{ BinaryReader::FromFile(aCookedPath) };
        if (!reader.IsValid())
        {
            return false;
        }

        AssetType type{};
        if (!AssetManager::ReadCookedHeader(reader, type) || type != AssetType::Texture)
        {
            NL_WARN(GCoreLogger, "cooked header mismatch for texture '{}'", aCookedPath.string());
            return false;
        }

        auto raw{ TextureSerializer::Read(reader) };
        if (!raw.IsValid())
        {
            return false;
        }
        
        raw.sourcePath = aSourcePath;

        const TextureKey key{ myStore.GetKey(aHash) };
        bool processOk{ false };
        myStore.Modify(key, [&](Texture& aTex)
        {
            processOk = TextureProcessor::Process(raw, aTex, *myGPU);
        });

        if (!processOk)
        {
            NL_ERROR(GCoreLogger, "texture processing failed for '{}'", aCookedPath.string());
            return false;
        }

        NL_INFO(GCoreLogger, "loaded texture from cooked '{}'", aCookedPath.string());
        return true;
    }
    
    bool TextureTypeHandler::IsUploadComplete(const uint64_t aHash) const
    {
        const TextureKey key{ myStore.GetKey(aHash) };
        bool ready{ false };
        myStore.Peek(key, [&](const Texture& aTex)
        {
            ready = myGPU->IsTextureReady(aTex.gpuTexture);
        });
        return ready;
    }

    bool TextureTypeHandler::OwnsHash(const uint64_t aHash) const
    {
        return myStore.Contains(aHash);
    }

    void TextureTypeHandler::SetReady(const uint64_t aHash)
    {
        myStore.SetState(myStore.GetKey(aHash), AssetState::Ready);
    }

    void TextureTypeHandler::SetFailed(const uint64_t aHash)
    {
        myStore.SetState(myStore.GetKey(aHash), AssetState::Failed);
    }

    AssetState TextureTypeHandler::GetState(const uint64_t aHash) const
    {
        return myStore.GetState(myStore.GetKey(aHash));
    }
    
    void TextureTypeHandler::InitializeFallback()
    {
        constexpr uint32_t W{ 4 };
        constexpr uint32_t H{ 4 };

        std::vector<uint8_t> pixels(W * H * 4);
        for (uint32_t y{ 0 }; y < H; ++y)
        {
            for (uint32_t x{ 0 }; x < W; ++x)
            {
                const bool checker{ (x + y) % 2 == 0 };
                const uint32_t i{ (y * W + x) * 4 };
                pixels[i + 0] = checker ? 0xFF : 0x00; // R
                pixels[i + 1] = 0x00;                  // G
                pixels[i + 2] = checker ? 0xFF : 0x00; // B
                pixels[i + 3] = 0xFF;                  // A
            }
        }

        RawTextureData raw;
        raw.width         = W;
        raw.height        = H;
        raw.mipLevels     = 1;
        raw.textureFormat = RHI::TextureFormat::RGBA8_UNORM;
        raw.mips.push_back(
        {
            .rowPitch   = W * 4,
            .slicePitch = W * H * 4,
            .pixels     = std::move(pixels)
        });
        
        raw.sourcePath = AssetPath{ "fallback" };

        const bool ok{ TextureProcessor::Process(raw, myFallback, *myGPU) };
        N_CORE_ASSERT(ok, "failed to create fallback texture");

        myGPU->GetDevice().FlushUploads();
        myFallback.state = AssetState::Ready;

        myStore.SetFallback(&myFallback);
        NL_INFO(GCoreLogger, "fallback texture created");
    }

    void TextureTypeHandler::DestroyAll(Graphics::GPUResourceManager& aGPU)
    {
        myStore.ForEach([&aGPU](const Texture& aTex)
        {
            if (aTex.gpuTexture.IsValid())
            {
                aGPU.DestroyTexture(aTex.gpuTexture);
            }
        });
        myStore.Clear();
    }
}