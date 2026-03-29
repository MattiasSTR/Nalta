#include "npch.h"
#include "Nalta/Assets/AssetManager.h"
#include "Nalta/Assets/Asset.h"
#include "Nalta/Assets/Importers/DDSImporter.h"
#include "Nalta/Core/BinaryIO.h"
#include "Nalta/Assets/Importers/IAssetImporter.h"
#include "Nalta/Assets/Importers/ObjImporter.h"
#include "Nalta/Assets/Importers/PipelineImporter.h"
#include "Nalta/Assets/Importers/TextureDescriptorImporter.h"
#include "Nalta/Assets/Importers/TextureImporter.h"
#include "Nalta/Assets/Processors/MeshProcessor.h"
#include "Nalta/Assets/Processors/PipelineProcessor.h"
#include "Nalta/Assets/Processors/RawAssetData.h"
#include "Nalta/Assets/Processors/TextureProcessor.h"
#include "Nalta/Assets/Serializers/MeshSerializer.h"
#include "Nalta/Assets/Serializers/PipelineSerializer.h"
#include "Nalta/Assets/Serializers/TextureSerializer.h"
#include "Nalta/Graphics/GraphicsSystem.h"
#include "Nalta/Graphics/Shader/ShaderCompiler.h"
#include "Nalta/Platform/IPlatformSystem.h"
#include "Nalta/Platform/PlatformFactory.h"

namespace Nalta
{
    namespace
    {
        std::filesystem::path GetCookedPath(const std::string& aSourcePath)
        {
            const StringID id{ aSourcePath };
            const std::string filename{ std::format("{:016x}.nasset", id.GetHash()) };
            return Paths::CookedDir() / filename;
        }
    }
    
    AssetManager::AssetManager() = default;
    AssetManager::~AssetManager() = default;
    
    void AssetManager::Initialize(GraphicsSystem* aGraphicsSystem, IPlatformSystem* aPlatformSystem)
    {
        NL_SCOPE_CORE("AssetManager");
        N_CORE_ASSERT(aGraphicsSystem, "AssetManager: null graphics system");

        myGraphicsSystem = aGraphicsSystem;
        myStop = false;
        myAssetThread = std::thread([this, aPlatformSystem]()
        {
            if (aPlatformSystem != nullptr) 
            {
                aPlatformSystem->SetCurrentThreadName("Asset");
            }
            AssetThreadLoop();
        });
        
        myRegistry.Initialize(Paths::CookedDir() / "AssetRegistry.json");
        
#ifndef N_SHIPPING
        myFileWatcher = CreateFileWatcher();
        myFileWatcher->Initialize();
        myFileWatcher->SetOnChangedCallback([this](const std::filesystem::path& aPath)
        {
            OnFileChanged(aPath);
        });
        myFileWatcher->Watch(Paths::EngineAssetDir());
#endif
        
        myImporterRegistry.Register(std::make_unique<ObjImporter>());
        myImporterRegistry.Register(std::make_unique<PipelineImporter>(myGraphicsSystem->GetShaderCompiler()));
        myImporterRegistry.Register(std::make_unique<TextureDescriptorImporter>());
        myImporterRegistry.Register(std::make_unique<TextureImporter>(".png"));
        myImporterRegistry.Register(std::make_unique<TextureImporter>(".jpg"));
        myImporterRegistry.Register(std::make_unique<TextureImporter>(".jpeg"));
        myImporterRegistry.Register(std::make_unique<TextureImporter>(".tga"));
        myImporterRegistry.Register(std::make_unique<DDSImporter>());
        
        mySerializerRegistry.Register(std::make_unique<MeshSerializer>());
        mySerializerRegistry.Register(std::make_unique<PipelineSerializer>());
        mySerializerRegistry.Register(std::make_unique<TextureSerializer>());
        
        myProcessorRegistry.Register(std::make_unique<MeshProcessor>());
        myProcessorRegistry.Register(std::make_unique<PipelineProcessor>());
        myProcessorRegistry.Register(std::make_unique<TextureProcessor>());

        NL_INFO(GCoreLogger, "initialized");
    }
    
    void AssetManager::Shutdown()
    {
        NL_SCOPE_CORE("AssetManager");

        // Signal asset thread to stop and wake it up
        myStop = true;
        myQueueCV.notify_all();

        if (myAssetThread.joinable())
        {
            myAssetThread.join();
        }
        
#ifndef N_SHIPPING
        if (myFileWatcher)
        {
            myFileWatcher->Shutdown();
            myFileWatcher.reset();
        }
#endif

        // Wait for GPU to finish before releasing assets
        if (myGraphicsSystem != nullptr)
        {
            myGraphicsSystem->WaitForGPU();
        }

        {
            std::lock_guard lock{ myAssetsMutex };
            myAssets.clear();
        }
        
        myRegistry.Shutdown();

        NL_INFO(GCoreLogger, "shutdown");
    }

    bool AssetManager::IsLoaded(const AssetPath& aPath) const
    {
        std::lock_guard lock{ myAssetsMutex };
        const auto it{ myAssets.find(aPath.GetHash()) };
        return it != myAssets.end() && it->second->IsReady();
    }
    
    void AssetManager::AssetThreadLoop()
    {
        NL_SCOPE_CORE("AssetThread");
        NL_INFO(GCoreLogger, "Asset thread started");

        while (!myStop)
        {
            // Check delayed reloads
            {
                std::lock_guard lock{ myDelayedReloadsMutex };
                const auto now{ std::chrono::steady_clock::now() };
                for (auto it{ myDelayedReloads.begin() }; it != myDelayedReloads.end();)
                {
                    if (now >= it->fireAt)
                    {
                        QueueReload(it->sourcePath);
                        it = myDelayedReloads.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }

            LoadRequest request;
            {
                std::unique_lock lock{ myQueueMutex };
                myQueueCV.wait_for(lock, std::chrono::milliseconds(100), [this]
                {
                    return !myLoadQueue.empty() || myStop;
                });

                if (myStop && myLoadQueue.empty()) break;
                if (myLoadQueue.empty()) continue;

                request = myLoadQueue.front();
                myLoadQueue.pop();
            }

            ProcessLoadRequest(request);
        }

        NL_INFO(GCoreLogger, "stopped");
    }

    void AssetManager::ProcessLoadRequest(const LoadRequest& aRequest)
    {
        NL_SCOPE_CORE("AssetManager");
        NL_TRACE(GCoreLogger, "processing '{}'", aRequest.path.GetPath());
    
        auto onFailure = [&]()
        {
            if (aRequest.isReload && aRequest.wasReady)
            {
                NL_WARN(GCoreLogger, "reload failed — keeping previous version of '{}'", aRequest.path.GetPath());
                aRequest.asset->SetState(AssetState::Ready);
            }
            else
            {
                aRequest.asset->SetState(AssetState::Failed);
            }
        };
    
        aRequest.asset->SetState(AssetState::Loading);
    
        const std::string sourcePath{ aRequest.path.GetPath() };
        const auto cookedPath{ GetCookedPath(sourcePath) };

#ifndef N_SHIPPING
        if (!myRegistry.NeedsRecook(sourcePath) && std::filesystem::exists(cookedPath))
        {
            NL_TRACE(GCoreLogger, "cooked asset is up to date, loading from cooked");
            if (LoadFromCooked(aRequest, cookedPath))
            {
                return;
            }
        
            NL_WARN(GCoreLogger, "cooked load failed — falling back to full import");
        }
#else
        if (!LoadFromCooked(aRequest, cookedPath))
        {
            NL_ERROR(GCoreLogger, "failed to load cooked '{}'", cookedPath.string());
            aRequest.asset->SetState(AssetState::Failed);
        }
        return;
#endif

        // Full import pipeline
        const IAssetImporter* importer{ myImporterRegistry.GetImporter(aRequest.path) };
        if (importer == nullptr)
        {
            NL_ERROR(GCoreLogger, "no importer for '{}'", sourcePath);
            onFailure();
            return;
        }
        
        aRequest.asset->SetState(AssetState::Processing);
        const auto rawData{ importer->Import(aRequest.path) };
        if (!rawData || !rawData->IsValid())
        {
            NL_ERROR(GCoreLogger, "import failed for '{}'", sourcePath);
            onFailure();
            return;
        }

        aRequest.asset->SetState(AssetState::Uploading);
        IAssetProcessor* processor{ myProcessorRegistry.GetProcessor(aRequest.asset->GetAssetType()) };
        
        if (processor == nullptr)
        {
            NL_ERROR(GCoreLogger, "no processor for asset type {}",static_cast<uint8_t>(aRequest.asset->GetAssetType()));
            onFailure();
            return;
        }
        
        if (!processor->Process(*rawData, *aRequest.asset, *myGraphicsSystem))
        {
            onFailure();
            return;
        }
        
        myGraphicsSystem->FlushUploads();

#ifndef N_SHIPPING
        const IAssetSerializer* serializer{ mySerializerRegistry.GetSerializer(aRequest.asset->GetAssetType()) };
        
        if (serializer != nullptr)
        {
            BinaryWriter writer;
        
            constexpr char magic[4]{ 'N', 'A', 'L', 'T' };
            writer.WriteBytes(std::span(reinterpret_cast<const uint8_t*>(magic), 4));
            writer.Write(uint32_t{ 1 });
            writer.Write(static_cast<uint8_t>(aRequest.asset->GetAssetType()));
            writer.Write(uint8_t{ 0 }); // padding
            writer.Write(uint8_t{ 0 }); // padding
            writer.Write(uint8_t{ 0 }); // padding
        
            serializer->Write(*rawData, writer);
        
            if (writer.SaveToFile(cookedPath))
            {
                NL_INFO(GCoreLogger, "cooked to '{}'", cookedPath.string());
        
                AssetRegistryEntry entry;
                entry.sourcePath = sourcePath;
                entry.cookedFile = cookedPath.filename().string();
                entry.assetType  = AssetTypeToString(aRequest.asset->GetAssetType());
                if (std::filesystem::exists(sourcePath))
                {
                    entry.lastModified = std::filesystem::last_write_time(sourcePath);
                }
        
                if (aRequest.asset->GetAssetType() == AssetType::Pipeline)
                {
                    const auto& pipelineData{ static_cast<const RawPipelineData&>(*rawData) };
                    if (!pipelineData.shaderPath.empty())
                    {
                        const auto pipelineDir{ std::filesystem::path(sourcePath).parent_path() };
                        const auto shaderAbsPath{ std::filesystem::weakly_canonical(pipelineDir / pipelineData.shaderPath) };
                        std::string normalized{ shaderAbsPath.string() };
                        std::ranges::replace(normalized, '\\', '/');
                        entry.dependencies.push_back(normalized);
                    }
                }
                
                if (aRequest.asset->GetAssetType() == AssetType::Texture)
                {
                    const auto& texData{ static_cast<const RawTextureData&>(*rawData) };
                    if (!texData.sourceImagePath.empty())
                    {
                        entry.dependencies.push_back(texData.sourceImagePath);
                    }
                }
        
                myRegistry.Register(entry);
                myRegistry.Save();
            }
            else
            {
                NL_WARN(GCoreLogger, "failed to cook '{}'", cookedPath.string());
            }
        }
#endif
    
        aRequest.asset->SetState(AssetState::Ready);
        NL_INFO(GCoreLogger, "loaded '{}'", sourcePath);
    }

    bool AssetManager::LoadFromCooked(const LoadRequest& aRequest, const std::filesystem::path& aCookedPath) const
    {
        NL_SCOPE_CORE("AssetManager");

        auto reader{ BinaryReader::FromFile(aCookedPath) };
        if (!reader.IsValid())
        {
            return false;
        }

        // Header
        char magic[4]{};
        reader.ReadBytes(std::span(reinterpret_cast<uint8_t*>(magic), 4));
        if (magic[0] != 'N' || magic[1] != 'A' || magic[2] != 'L' || magic[3] != 'T')
        {
            NL_ERROR(GCoreLogger, "invalid magic in '{}'", aCookedPath.string());
            return false;
        }

        const uint32_t version{ reader.Read<uint32_t>() };
        const auto assetType{ static_cast<AssetType>(reader.Read<uint8_t>()) };
        reader.Read<uint8_t>(); // padding
        reader.Read<uint8_t>(); // padding
        reader.Read<uint8_t>(); // padding

        if (version != 1)
        {
            NL_WARN(GCoreLogger, "version mismatch — will recook");
            return false;
        }

        IAssetSerializer* serializer{ mySerializerRegistry.GetSerializer(assetType) };
        if (serializer == nullptr)
        {
            NL_ERROR(GCoreLogger, "no serializer for asset type {}", static_cast<uint8_t>(assetType));
            return false;
        }

        auto rawData{ serializer->Read(reader) };
        if (!rawData || !rawData->IsValid())
        {
            NL_ERROR(GCoreLogger, "failed to deserialize '{}'", aCookedPath.string());
            return false;
        }

        rawData->sourcePath = aRequest.path;

        const IAssetProcessor* processor{ myProcessorRegistry.GetProcessor(assetType) };
        if (processor == nullptr)
        {
            NL_ERROR(GCoreLogger, "no processor for asset type {}", static_cast<uint8_t>(assetType));
            return false;
        }

        aRequest.asset->SetState(AssetState::Uploading);
        if (!processor->Process(*rawData, *aRequest.asset, *myGraphicsSystem))
        {
            aRequest.asset->SetState(AssetState::Failed);
            return false;
        }

        myGraphicsSystem->FlushUploads();

        aRequest.asset->SetState(AssetState::Ready);
        NL_INFO(GCoreLogger, "loaded from cooked '{}'", aCookedPath.string());
        return true;
    }

    void AssetManager::OnFileChanged(const std::filesystem::path& aPath)
    {
        const std::string pathStr{ aPath.string() };
        NL_INFO(GCoreLogger, "file changed: '{}'", pathStr);

        // Reload the changed asset directly
        ReloadAsset(pathStr);

        // Reload all assets that depend on it
        const auto dependents{ myRegistry.FindDependents(pathStr) };
        for (const auto& dependent : dependents)
        {
            NL_INFO(GCoreLogger, "reloading dependent: '{}'", dependent);
            ReloadAsset(dependent);
        }
    }

    void AssetManager::ReloadAsset(const std::string& aSourcePath)
    {
        // Schedule with delay to let editor finish writing
        std::lock_guard lock{ myDelayedReloadsMutex };
    
        // Check if already scheduled - update fire time
        for (auto& delayed : myDelayedReloads)
        {
            if (delayed.sourcePath == aSourcePath)
            {
                delayed.fireAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
                return;
            }
        }

        myDelayedReloads.push_back(
        {
            .sourcePath = aSourcePath,
            .fireAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(200)
        });

        myQueueCV.notify_one();
    }

    void AssetManager::QueueReload(const std::string& aSourcePath)
    {
        std::shared_ptr<Asset> asset;
        bool wasReady{ false };
        {
            std::lock_guard lock{ myAssetsMutex };
            const StringID id{ aSourcePath };
            const auto it{ myAssets.find(id.GetHash()) };
            if (it == myAssets.end())
            {
                return;
            }
            asset = it->second;
            wasReady = asset->IsReady();
        }

        const auto cookedPath{ GetCookedPath(aSourcePath) };
        if (std::filesystem::exists(cookedPath))
        {
            std::filesystem::remove(cookedPath);
        }

        asset->SetState(AssetState::Requested);
        {
            std::lock_guard lock{ myQueueMutex };
            myLoadQueue.push({ asset, AssetPath(aSourcePath), true, wasReady });
        }
        myQueueCV.notify_one();
    }
}
