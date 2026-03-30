#include "npch.h"
#include "Nalta/Assets/AssetManager.h"
#include "Nalta/Assets/Importers/ObjImporter.h"
#include "Nalta/Assets/Importers/TextureImporter.h"
#include "Nalta/Assets/Importers/DDSImporter.h"
#include "Nalta/Assets/Importers/PipelineImporter.h"
#include "Nalta/Assets/Importers/TextureDescriptorImporter.h"
#include "Nalta/Assets/Serializers/MeshSerializer.h"
#include "Nalta/Assets/Serializers/TextureSerializer.h"
#include "Nalta/Assets/Serializers/PipelineSerializer.h"
#include "Nalta/Assets/Processors/MeshProcessor.h"
#include "Nalta/Assets/Processors/TextureProcessor.h"
#include "Nalta/Assets/Processors/PipelineProcessor.h"
#include "Nalta/Assets/RawAssetData.h"
#include "Nalta/Core/BinaryIO.h"
#include "Nalta/Core/Paths.h"
#include "Nalta/Graphics/GraphicsSystem.h"
#include "Nalta/Graphics/Shader/ShaderCompiler.h"
#include "Nalta/Graphics/Shader/ShaderDesc.h"
#include "Nalta/Platform/IPlatformSystem.h"
#include "Nalta/Platform/PlatformFactory.h"


namespace Nalta
{
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
        
        InitializeFallbacks();
        
        NL_INFO(GCoreLogger, "initialized");
    }

    void AssetManager::Shutdown()
    {
        NL_SCOPE_CORE("AssetManager");

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
        
        if (myGraphicsSystem != nullptr)
        {
            myGraphicsSystem->WaitForGPU();
        }
        
        {
            std::lock_guard lock{ myMeshMutex };
            myMeshes.clear();
        }
        {
            std::lock_guard lock{ myTextureMutex };
            myTextures.clear();
        }
        {
            std::lock_guard lock{ myPipelineMutex };
            myPipelines.clear();
        }

        myRegistry.Shutdown();
        NL_INFO(GCoreLogger, "shutdown");
    }
    
    MeshHandle AssetManager::RequestMesh(const AssetPath& aPath)
    {
        return RequestMeshInternal(aPath, false);
    }

    TextureHandle AssetManager::RequestTexture(const AssetPath& aPath)
    {
        return RequestTextureInternal(aPath, false);
    }

    PipelineHandle AssetManager::RequestPipeline(const AssetPath& aPath)
    {
        return RequestPipelineInternal(aPath, false);
    }

    const Mesh* AssetManager::GetMesh(const MeshHandle aHandle) const
    {
        if (aHandle.IsValid())
        {
            std::lock_guard lock{ myMeshMutex };
            const auto it{ myMeshes.find(aHandle.id) };
            if (it != myMeshes.end() && it->second.state == AssetState::Ready)
            {
                return &it->second;
            }
        }
        return &myFallbackMesh;
    }

    const Texture* AssetManager::GetTexture(const TextureHandle aHandle) const
    {
        if (aHandle.IsValid())
        {
            std::lock_guard lock{ myTextureMutex };
            const auto it{ myTextures.find(aHandle.id) };
            if (it != myTextures.end() && it->second.state == AssetState::Ready)
            {
                return &it->second;
            }
        }
        return &myFallbackTexture;
    }

    const Pipeline* AssetManager::GetPipeline(const PipelineHandle aHandle) const
    {
        if (aHandle.IsValid())
        {
            std::lock_guard lock{ myPipelineMutex };
            const auto it{ myPipelines.find(aHandle.id) };
            if (it != myPipelines.end() && it->second.state == AssetState::Ready)
            {
                return &it->second;
            }
        }
        //return &myFallbackPipeline;
        return nullptr;
    }

    MeshHandle AssetManager::RequestMeshInternal(const AssetPath& aPath, const bool aIsReload)
    {
        N_CORE_ASSERT(!aPath.IsEmpty(), "empty asset path");
        const uint64_t id{ aPath.GetHash() };

        {
            std::lock_guard lock{ myMeshMutex };
            if (!aIsReload)
            {
                if (myMeshes.contains(id))
                {
                    return MeshHandle{ id };
                }
            }

            if (!aIsReload)
            {
                myMeshes[id].state = AssetState::Requested;
            }
        }

        {
            std::lock_guard lock{ myQueueMutex };
            myLoadQueue.push({ id, aPath, AssetType::Mesh, aIsReload });
        }
        myQueueCV.notify_one();

        NL_TRACE(GCoreLogger, "requested mesh '{}'", aPath.GetPath());
        return MeshHandle{ id };
    }

    TextureHandle AssetManager::RequestTextureInternal(const AssetPath& aPath, const bool aIsReload)
    {
        N_CORE_ASSERT(!aPath.IsEmpty(), "empty asset path");
        const uint64_t id{ aPath.GetHash() };

        {
            std::lock_guard lock{ myTextureMutex };
            if (!aIsReload && myTextures.contains(id))
            {
                return TextureHandle{ id };
            }

            if (!aIsReload)
            {
                myTextures[id].state = AssetState::Requested;
            }
        }

        {
            std::lock_guard lock{ myQueueMutex };
            myLoadQueue.push({ id, aPath, AssetType::Texture, aIsReload });
        }
        myQueueCV.notify_one();

        NL_TRACE(GCoreLogger, "requested texture '{}'", aPath.GetPath());
        return TextureHandle{ id };
    }

    PipelineHandle AssetManager::RequestPipelineInternal(const AssetPath& aPath, const bool aIsReload)
    {
        N_CORE_ASSERT(!aPath.IsEmpty(), "empty asset path");
        const uint64_t id{ aPath.GetHash() };

        {
            std::lock_guard lock{ myPipelineMutex };
            if (!aIsReload && myPipelines.contains(id))
            {
                return PipelineHandle{ id };
            }

            if (!aIsReload)
            {
                myPipelines[id].state = AssetState::Requested;
            }
        }

        {
            std::lock_guard lock{ myQueueMutex };
            myLoadQueue.push({ id, aPath, AssetType::Pipeline, aIsReload });
        }
        myQueueCV.notify_one();

        NL_TRACE(GCoreLogger, "requested pipeline '{}'", aPath.GetPath());
        return PipelineHandle{ id };
    }

    void AssetManager::AssetThreadLoop()
    {
        NL_SCOPE_CORE("AssetThread");
        NL_INFO(GCoreLogger, "asset thread started");
        
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

                if (myStop && myLoadQueue.empty())
                {
                    break;
                }
                if (myLoadQueue.empty())
                {
                    continue;
                }

                request = myLoadQueue.front();
                myLoadQueue.pop();
            }

            ProcessLoadRequest(request);
        }
        
        NL_INFO(GCoreLogger, "asset thread stopped");
    }

    void AssetManager::ProcessLoadRequest(const LoadRequest& aRequest)
    {
        bool success{ false };

        switch (aRequest.type)
        {
            case AssetType::Mesh: success = LoadMesh(aRequest); break;
            case AssetType::Texture: success = LoadTexture(aRequest); break;
            case AssetType::Pipeline: success = LoadPipeline(aRequest); break;
        }
        
        // Flush only when the queue is drained - batches all pending uploads together
        // TODO: MAKE THIS SMARTER, What if we queue 1000 assets? needs to be chunked
        {
            std::lock_guard lock{ myQueueMutex };
            if (myLoadQueue.empty())
            {
                myGraphicsSystem->FlushUploads();
                PromotePendingAssets();
            }
        }

        if (!success)
        {
            if (aRequest.isReload && aRequest.wasReady)
            {
                NL_WARN(GCoreLogger, "reload failed - keeping previous version of '{}'", aRequest.path.GetPath());
                // Restore ready state so the old GPU data keeps being used
                switch (aRequest.type)
                {
                    case AssetType::Mesh: SetMeshState(aRequest.id, AssetState::Ready, aRequest.isReload); break;
                    case AssetType::Texture: SetTextureState(aRequest.id, AssetState::Ready, aRequest.isReload); break;
                    case AssetType::Pipeline: SetPipelineState(aRequest.id, AssetState::Ready, aRequest.isReload); break;
                }
            }
            else
            {
                switch (aRequest.type)
                {
                    case AssetType::Mesh: SetMeshState(aRequest.id, AssetState::Failed, aRequest.isReload); break;
                    case AssetType::Texture: SetTextureState(aRequest.id, AssetState::Failed, aRequest.isReload); break;
                    case AssetType::Pipeline: SetPipelineState(aRequest.id, AssetState::Failed, aRequest.isReload); break;
                }
            }
        }
    }

    bool AssetManager::LoadMesh(const LoadRequest& aRequest)
    {
        SetMeshState(aRequest.id, AssetState::Loading, aRequest.isReload);

        const auto cookedPath{ GetCookedPath(aRequest.path) };

#ifndef N_SHIPPING
        if (!myRegistry.NeedsRecook(aRequest.path) && std::filesystem::exists(cookedPath))
        {
            NL_TRACE(GCoreLogger, "loading mesh from cooked");
            if (LoadMeshFromCooked(aRequest, cookedPath))
            {
                return true;
            }

            NL_WARN(GCoreLogger, "cooked load failed — falling back to full import");
        }

        return CookAndProcessMesh(aRequest, aRequest.path);
#else
        if (!LoadMeshFromCooked(aRequest, cookedPath))
        {
            NL_ERROR(GCoreLogger, "failed to load cooked mesh '{}'", cookedPath.string());
            return false;
        }
        return true;
#endif
    }

    bool AssetManager::LoadTexture(const LoadRequest& aRequest)
    {
        SetTextureState(aRequest.id, AssetState::Loading, aRequest.isReload);

        const auto cookedPath{ GetCookedPath(aRequest.path) };

#ifndef N_SHIPPING
        if (!myRegistry.NeedsRecook(aRequest.path) && std::filesystem::exists(cookedPath))
        {
            NL_TRACE(GCoreLogger, "loading texture from cooked");
            if (LoadTextureFromCooked(aRequest, cookedPath))
            {
                return true;
            }

            NL_WARN(GCoreLogger, "cooked load failed — falling back to full import");
        }

        return CookAndProcessTexture(aRequest, aRequest.path);
#else
        if (!LoadTextureFromCooked(aRequest, cookedPath))
        {
            NL_ERROR(GCoreLogger, "failed to load cooked texture '{}'", cookedPath.string());
            return false;
        }
        return true;
#endif
    }

    bool AssetManager::LoadPipeline(const LoadRequest& aRequest)
    {
        SetPipelineState(aRequest.id, AssetState::Loading, aRequest.isReload);

        const auto cookedPath{ GetCookedPath(aRequest.path) };

#ifndef N_SHIPPING
        if (!myRegistry.NeedsRecook(aRequest.path) && std::filesystem::exists(cookedPath))
        {
            NL_TRACE(GCoreLogger, "loading pipeline from cooked");
            if (LoadPipelineFromCooked(aRequest, cookedPath))
            {
                return true;
            }

            NL_WARN(GCoreLogger, "cooked load failed — falling back to full import");
        }

        return CookAndProcessPipeline(aRequest, aRequest.path);
#else
        if (!LoadPipelineFromCooked(aRequest, cookedPath))
        {
            NL_ERROR(GCoreLogger, "failed to load cooked pipeline '{}'", cookedPath.string());
            return false;
        }
        return true;
#endif
    }

    bool AssetManager::CookAndProcessMesh(const LoadRequest& aRequest, const AssetPath& aPath)
    {
        const IAssetImporter* importer{ myImporterRegistry.GetImporter(aPath) };
        if (importer == nullptr)
        {
            NL_ERROR(GCoreLogger, "no importer for '{}'", aPath.GetPath());
            return false;
        }

        SetMeshState(aRequest.id, AssetState::Processing, aRequest.isReload);
        const auto rawBase{ importer->Import(aPath) };
        if (!rawBase || !rawBase->IsValid())
        {
            NL_ERROR(GCoreLogger, "import failed for '{}'", aPath.GetPath());
            return false;
        }

        auto& raw{ static_cast<RawMeshData&>(*rawBase) };

        SetMeshState(aRequest.id, AssetState::Uploading, aRequest.isReload);
        {
            std::lock_guard lock{ myMeshMutex };
            if (!MeshProcessor::Process(raw, myMeshes[aRequest.id], *myGraphicsSystem))
            {
                return false;
            }
        }

#ifndef N_SHIPPING
        BinaryWriter writer;
        WriteCookedHeader(writer, AssetType::Mesh);
        MeshSerializer::Write(raw, writer);

        const auto cookedPath{ GetCookedPath(aPath) };
        if (writer.SaveToFile(cookedPath))
        {
            RegisterCookedEntry(aPath, cookedPath, AssetType::Mesh, {});
            NL_INFO(GCoreLogger, "cooked mesh to '{}'", cookedPath.string());
        }
#endif
        
        NL_INFO(GCoreLogger, "loaded mesh '{}'", aPath.GetPath());
        return true;
    }

    bool AssetManager::CookAndProcessTexture(const LoadRequest& aRequest, const AssetPath& aPath)
    {
        const IAssetImporter* importer{ myImporterRegistry.GetImporter(aPath) };
        if (importer == nullptr)
        {
            NL_ERROR(GCoreLogger, "no importer for '{}'", aPath.GetPath());
            return false;
        }

        SetTextureState(aRequest.id, AssetState::Processing, aRequest.isReload);
        const auto rawBase{ importer->Import(aPath) };
        if (!rawBase || !rawBase->IsValid())
        {
            NL_ERROR(GCoreLogger, "import failed for '{}'", aPath.GetPath());
            return false;
        }

        auto& raw{ static_cast<RawTextureData&>(*rawBase) };

        SetTextureState(aRequest.id, AssetState::Uploading, aRequest.isReload);
        {
            std::lock_guard lock{ myTextureMutex };
            if (!TextureProcessor::Process(raw, myTextures[aRequest.id], *myGraphicsSystem))
            {
                return false;
            }
        }

#ifndef N_SHIPPING
        std::vector<std::string> deps;
        if (!raw.sourceImagePath.empty())
        {
            deps.push_back(raw.sourceImagePath);
        }

        BinaryWriter writer;
        WriteCookedHeader(writer, AssetType::Texture);
        TextureSerializer::Write(raw, writer);

        const auto cookedPath{ GetCookedPath(aPath) };
        if (writer.SaveToFile(cookedPath))
        {
            RegisterCookedEntry(aPath, cookedPath, AssetType::Texture, deps);
            NL_INFO(GCoreLogger, "cooked texture to '{}'", cookedPath.string());
        }
#endif
        
        NL_INFO(GCoreLogger, "loaded texture '{}'", aPath.GetPath());
        return true;
    }

    bool AssetManager::CookAndProcessPipeline(const LoadRequest& aRequest, const AssetPath& aPath)
    {
        const IAssetImporter* importer{ myImporterRegistry.GetImporter(aPath) };
        if (importer == nullptr)
        {
            NL_ERROR(GCoreLogger, "no importer for '{}'", aPath.GetPath());
            return false;
        }

        SetPipelineState(aRequest.id, AssetState::Processing, aRequest.isReload);
        const auto rawBase{ importer->Import(aPath) };
        if (!rawBase || !rawBase->IsValid())
        {
            NL_ERROR(GCoreLogger, "import failed for '{}'", aPath.GetPath());
            return false;
        }

        auto& raw{ static_cast<RawPipelineData&>(*rawBase) };

        SetPipelineState(aRequest.id, AssetState::Uploading, aRequest.isReload);
        {
            std::lock_guard lock{ myPipelineMutex };
            if (!PipelineProcessor::Process(raw, myPipelines[aRequest.id], *myGraphicsSystem))
            {
                return false;
            }
        }

#ifndef N_SHIPPING
        std::vector<std::string> deps;
        if (!raw.shaderPath.empty())
        {
            const auto pipelineDir{ std::filesystem::path(aPath.GetPath()).parent_path() };
            auto shaderAbs{ std::filesystem::weakly_canonical(pipelineDir / raw.shaderPath) };
            std::string normalized{ shaderAbs.string() };
            std::ranges::replace(normalized, '\\', '/');
            deps.push_back(normalized);
        }

        BinaryWriter writer;
        WriteCookedHeader(writer, AssetType::Pipeline);
        PipelineSerializer::Write(raw, writer);

        const auto cookedPath{ GetCookedPath(aPath) };
        if (writer.SaveToFile(cookedPath))
        {
            RegisterCookedEntry(aPath, cookedPath, AssetType::Pipeline, deps);
            NL_INFO(GCoreLogger, "cooked pipeline to '{}'", cookedPath.string());
        }
#endif
        
        NL_INFO(GCoreLogger, "loaded pipeline '{}'", aPath.GetPath());
        return true;
    }

    bool AssetManager::LoadMeshFromCooked(const LoadRequest& aRequest, const std::filesystem::path& aCookedPath)
    {
        auto reader{ BinaryReader::FromFile(aCookedPath) };
        if (!reader.IsValid())
        {
            return false;
        }

        AssetType type{};
        if (!ReadCookedHeader(reader, type) || type != AssetType::Mesh)
        {
            NL_WARN(GCoreLogger, "cooked header mismatch for mesh '{}'", aCookedPath.string());
            return false;
        }

        const auto raw{ MeshSerializer::Read(reader) };
        if (!raw.IsValid())
        {
            return false;
        }

        SetMeshState(aRequest.id, AssetState::Uploading, aRequest.isReload);
        {
            std::lock_guard lock{ myMeshMutex };
            if (!MeshProcessor::Process(raw, myMeshes[aRequest.id], *myGraphicsSystem))
            {
                return false;
            }
        }
        
        NL_INFO(GCoreLogger, "loaded mesh from cooked '{}'", aCookedPath.string());
        return true;
    }

    bool AssetManager::LoadTextureFromCooked(const LoadRequest& aRequest, const std::filesystem::path& aCookedPath)
    {
        auto reader{ BinaryReader::FromFile(aCookedPath) };
        if (!reader.IsValid())
        {
            return false;
        }

        AssetType type{};
        if (!ReadCookedHeader(reader, type) || type != AssetType::Texture)
        {
            NL_WARN(GCoreLogger, "cooked header mismatch for texture '{}'", aCookedPath.string());
            return false;
        }

        const auto raw{ TextureSerializer::Read(reader) };
        if (!raw.IsValid())
        {
            return false;
        }
        
        SetTextureState(aRequest.id, AssetState::Uploading, aRequest.isReload);
        {
            std::lock_guard lock{ myTextureMutex };
            if (!TextureProcessor::Process(raw, myTextures[aRequest.id], *myGraphicsSystem))
            {
                return false;
            }
        }
        
        NL_INFO(GCoreLogger, "loaded texture from cooked '{}'", aCookedPath.string());
        return true;
    }

    bool AssetManager::LoadPipelineFromCooked(const LoadRequest& aRequest, const std::filesystem::path& aCookedPath)
    {
        auto reader{ BinaryReader::FromFile(aCookedPath) };
        if (!reader.IsValid())
        {
            return false;
        }

        AssetType type{};
        if (!ReadCookedHeader(reader, type) || type != AssetType::Pipeline)
        {
            NL_WARN(GCoreLogger, "cooked header mismatch for pipeline '{}'", aCookedPath.string());
            return false;
        }

        const auto raw{ PipelineSerializer::Read(reader) };
        if (!raw.IsValid())
        {
            return false;
        }
        
        SetPipelineState(aRequest.id, AssetState::Uploading, aRequest.isReload);
        {
            std::lock_guard lock{ myPipelineMutex };
            if (!PipelineProcessor::Process(raw, myPipelines[aRequest.id], *myGraphicsSystem))
            {
                return false;
            }
        }
        
        NL_INFO(GCoreLogger, "loaded pipeline from cooked '{}'", aCookedPath.string());
        return true;
    }

    void AssetManager::OnFileChanged(const std::filesystem::path& aPath)
    {
        const AssetPath path{ aPath };
        NL_INFO(GCoreLogger, "file changed: '{}'", path.GetPath());

        DebounceReload(path.GetPath());

        for (const auto& dependent : myRegistry.FindDependents(path))
        {
            NL_INFO(GCoreLogger, "reloading dependent: '{}'", dependent);
            DebounceReload(dependent);
        }
    }

    void AssetManager::DebounceReload(const std::string& aSourcePath)
    {
        std::lock_guard lock{ myDelayedReloadsMutex };

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
        const AssetPath path{ aSourcePath };
        const uint64_t id{ path.GetHash() };

        // Figure out which type this path maps to and whether it's loaded
        AssetType type{};
        bool wasReady{ false };
        bool found{ false };
        
        {
            std::lock_guard lock{ myMeshMutex };
            if (myMeshes.contains(id))
            {
                wasReady = myMeshes[id].state == AssetState::Ready;
                type = AssetType::Mesh;
                found  = true;
            }
        }

        if (!found)
        {
            std::lock_guard lock{ myTextureMutex };
            if (myTextures.contains(id))
            {
                wasReady = myTextures[id].state == AssetState::Ready;
                type = AssetType::Texture;
                found = true;
            }
        }

        if (!found)
        {
            std::lock_guard lock{ myPipelineMutex };
            if (myPipelines.contains(id))
            {
                wasReady = myPipelines[id].state == AssetState::Ready;
                type = AssetType::Pipeline;
                found = true;
            }
        }
        
        if (!found)
        {
            NL_TRACE(GCoreLogger, "file changed but '{}' is not loaded — skipping reload", aSourcePath);
            return;
        }

        // Delete stale cooked file
        const auto cookedPath{ GetCookedPath(path) };
        if (std::filesystem::exists(cookedPath))
        {
            std::filesystem::remove(cookedPath);
        }

        {
            std::lock_guard lock{ myQueueMutex };
            myLoadQueue.push({ id, path, type, true, wasReady });
        }
        myQueueCV.notify_one();
    }

    std::filesystem::path AssetManager::GetCookedPath(const AssetPath& aPath)
    {
        const std::string filename{ std::format("{:016x}.nasset", aPath.GetHash()) };
        return Paths::CookedDir() / filename;
    }

    void AssetManager::SetMeshState(const uint64_t aId, const AssetState aState, const bool aIsReload)
    {
        if (aIsReload && aState != AssetState::Ready && aState != AssetState::Failed)
        {
            return;
        }

        std::lock_guard lock{ myMeshMutex };
        myMeshes[aId].state = aState;
    }

    void AssetManager::SetTextureState(const uint64_t aId, const AssetState aState, const bool aIsReload)
    {
        if (aIsReload && aState != AssetState::Ready && aState != AssetState::Failed)
        {
            return;
        }
        
        std::lock_guard lock{ myTextureMutex };
        myTextures[aId].state = aState;
    }

    void AssetManager::SetPipelineState(const uint64_t aId, const AssetState aState, const bool aIsReload)
    {
        if (aIsReload && aState != AssetState::Ready && aState != AssetState::Failed)
        {
            return;
        }
        
        std::lock_guard lock{ myPipelineMutex };
        myPipelines[aId].state = aState;
    }

    void AssetManager::PromotePendingAssets()
    {
        {
            std::lock_guard lock{ myMeshMutex };
            for (auto& mesh : myMeshes | std::views::values)
            {
                if (mesh.state == AssetState::Uploading)
                {
                    mesh.state = AssetState::Ready;
                }
            }
        }
        {
            std::lock_guard lock{ myTextureMutex };
            for (auto& texture : myTextures | std::views::values)
            {
                if (texture.state == AssetState::Uploading)
                {
                    texture.state = AssetState::Ready;
                }
            }
        }
        {
            std::lock_guard lock{ myPipelineMutex };
            for (auto& pipeline : myPipelines | std::views::values)
            {
                if (pipeline.state == AssetState::Uploading)
                {
                    pipeline.state = AssetState::Ready;
                }
            }
        }
    }

    void AssetManager::WriteCookedHeader(BinaryWriter& aWriter, AssetType aType)
    {
        constexpr char magic[4]{ 'N', 'A', 'L', 'T' };
        aWriter.WriteBytes(std::span(reinterpret_cast<const uint8_t*>(magic), 4));
        aWriter.Write(uint32_t{ 1 }); // version
        aWriter.Write(static_cast<uint8_t>(aType));
        aWriter.Write(uint8_t{ 0 }); // padding
        aWriter.Write(uint8_t{ 0 }); // padding
        aWriter.Write(uint8_t{ 0 }); // padding
    }

    bool AssetManager::ReadCookedHeader(BinaryReader& aReader, AssetType& aOutType) const
    {
        char magic[4]{};
        aReader.ReadBytes(std::span(reinterpret_cast<uint8_t*>(magic), 4));
        if (magic[0] != 'N' || magic[1] != 'A' || magic[2] != 'L' || magic[3] != 'T')
        {
            NL_ERROR(GCoreLogger, "invalid cooked asset magic");
            return false;
        }

        const uint32_t version{ aReader.Read<uint32_t>() };
        if (version != 1)
        {
            NL_WARN(GCoreLogger, "cooked asset version mismatch — will recook");
            return false;
        }

        aOutType = static_cast<AssetType>(aReader.Read<uint8_t>());
        aReader.Read<uint8_t>(); // padding
        aReader.Read<uint8_t>(); // padding
        aReader.Read<uint8_t>(); // padding
        return true;
    }

    void AssetManager::RegisterCookedEntry(const AssetPath& aPath, const std::filesystem::path& aCookedPath, AssetType aType, const std::vector<std::string>& aDependencies)
    {
        static constexpr std::string_view typeNames[]{ "Mesh", "Texture", "Pipeline" };

        AssetRegistryEntry entry;
        entry.sourcePath = aPath.GetPath();
        entry.cookedFile = aCookedPath.filename().string();
        entry.assetType = typeNames[static_cast<uint8_t>(aType)];
        entry.dependencies = aDependencies;

        if (std::filesystem::exists(aPath.GetPath()))
        {
            entry.lastModified = std::filesystem::last_write_time(aPath.GetPath());
        }

        myRegistry.Register(entry);
        myRegistry.Save();
    }

    void AssetManager::InitializeFallbacks()
    {
        InitializeFallbackMesh();
        InitializeFallbackTexture();
        InitializeFallbackPipeline();
    
        NL_INFO(GCoreLogger, "fallbacks initialized");
    }

    void AssetManager::InitializeFallbackMesh()
    {
        const std::vector<MeshVertex> vertices
        {
            // Front
            { .position = float3{-0.5f, -0.5f, -0.5f}, .normal = float3{ 0.f,  0.f, -1.f}, .tangent = float4{1.f, 0.f, 0.f, 1.f}, .texCoord = float2{0.f, 1.f} },
            { .position = float3{ 0.5f, -0.5f, -0.5f}, .normal = float3{ 0.f,  0.f, -1.f}, .tangent = float4{1.f, 0.f, 0.f, 1.f}, .texCoord = float2{1.f, 1.f} },
            { .position = float3{ 0.5f,  0.5f, -0.5f}, .normal = float3{ 0.f,  0.f, -1.f}, .tangent = float4{1.f, 0.f, 0.f, 1.f}, .texCoord = float2{1.f, 0.f} },
            { .position = float3{-0.5f,  0.5f, -0.5f}, .normal = float3{ 0.f,  0.f, -1.f}, .tangent = float4{1.f, 0.f, 0.f, 1.f}, .texCoord = float2{0.f, 0.f} },
            // Back
            { .position = float3{ 0.5f, -0.5f,  0.5f}, .normal = float3{ 0.f,  0.f,  1.f}, .tangent = float4{-1.f, 0.f, 0.f, 1.f}, .texCoord = float2{0.f, 1.f} },
            { .position = float3{-0.5f, -0.5f,  0.5f}, .normal = float3{ 0.f,  0.f,  1.f}, .tangent = float4{-1.f, 0.f, 0.f, 1.f}, .texCoord = float2{1.f, 1.f} },
            { .position = float3{-0.5f,  0.5f,  0.5f}, .normal = float3{ 0.f,  0.f,  1.f}, .tangent = float4{-1.f, 0.f, 0.f, 1.f}, .texCoord = float2{1.f, 0.f} },
            { .position = float3{ 0.5f,  0.5f,  0.5f}, .normal = float3{ 0.f,  0.f,  1.f}, .tangent = float4{-1.f, 0.f, 0.f, 1.f}, .texCoord = float2{0.f, 0.f} },
            // Left
            { .position = float3{-0.5f, -0.5f,  0.5f}, .normal = float3{-1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f, -1.f, 1.f}, .texCoord = float2{0.f, 1.f} },
            { .position = float3{-0.5f, -0.5f, -0.5f}, .normal = float3{-1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f, -1.f, 1.f}, .texCoord = float2{1.f, 1.f} },
            { .position = float3{-0.5f,  0.5f, -0.5f}, .normal = float3{-1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f, -1.f, 1.f}, .texCoord = float2{1.f, 0.f} },
            { .position = float3{-0.5f,  0.5f,  0.5f}, .normal = float3{-1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f, -1.f, 1.f}, .texCoord = float2{0.f, 0.f} },
            // Right
            { .position = float3{ 0.5f, -0.5f, -0.5f}, .normal = float3{ 1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f,  1.f, 1.f}, .texCoord = float2{0.f, 1.f} },
            { .position = float3{ 0.5f, -0.5f,  0.5f}, .normal = float3{ 1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f,  1.f, 1.f}, .texCoord = float2{1.f, 1.f} },
            { .position = float3{ 0.5f,  0.5f,  0.5f}, .normal = float3{ 1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f,  1.f, 1.f}, .texCoord = float2{1.f, 0.f} },
            { .position = float3{ 0.5f,  0.5f, -0.5f}, .normal = float3{ 1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f,  1.f, 1.f}, .texCoord = float2{0.f, 0.f} },
            // Top 
            { .position = float3{-0.5f,  0.5f, -0.5f}, .normal = float3{ 0.f,  1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{0.f, 1.f} },
            { .position = float3{ 0.5f,  0.5f, -0.5f}, .normal = float3{ 0.f,  1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{1.f, 1.f} },
            { .position = float3{ 0.5f,  0.5f,  0.5f}, .normal = float3{ 0.f,  1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{1.f, 0.f} },
            { .position = float3{-0.5f,  0.5f,  0.5f}, .normal = float3{ 0.f,  1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{0.f, 0.f} },
            // Bottom
            { .position = float3{-0.5f, -0.5f,  0.5f}, .normal = float3{ 0.f, -1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{0.f, 1.f} },
            { .position = float3{ 0.5f, -0.5f,  0.5f}, .normal = float3{ 0.f, -1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{1.f, 1.f} },
            { .position = float3{ 0.5f, -0.5f, -0.5f}, .normal = float3{ 0.f, -1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{1.f, 0.f} },
            { .position = float3{-0.5f, -0.5f, -0.5f}, .normal = float3{ 0.f, -1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{0.f, 0.f} },
        };
    
        const std::vector<uint32_t> indices
        {
            0,  2,  1,  2,  0,  3,  // front
            4,  6,  5,  6,  4,  7,  // back
            8, 10,  9, 10,  8, 11,  // left
           12, 14, 13, 14, 12, 15,  // right
           16, 18, 17, 18, 16, 19,  // top
           20, 22, 21, 22, 20, 23,  // bottom
        };
    
        RawMeshData raw;
        for (const auto& v : vertices)
        {
            raw.vertices.push_back(
            {
                .position = { v.position.x, v.position.y, v.position.z },
                .normal   = { v.normal.x,   v.normal.y,   v.normal.z   },
                .texCoord = { v.texCoord.x, v.texCoord.y                },
                .tangent  = { v.tangent.x,  v.tangent.y,  v.tangent.z, v.tangent.w }
            });
        }
        raw.indices = indices;
        raw.submeshes.push_back(
        {
            .name        = "fallback",
            .vertexOffset = 0,
            .vertexCount  = static_cast<uint32_t>(vertices.size()),
            .indexOffset  = 0,
            .indexCount   = static_cast<uint32_t>(indices.size()),
            .materialIndex = 0
        });
        raw.boundsMin[0] = raw.boundsMin[1] = raw.boundsMin[2] = -0.5f;
        raw.boundsMax[0] = raw.boundsMax[1] = raw.boundsMax[2] =  0.5f;
    
        const bool ok{ MeshProcessor::Process(raw, myFallbackMesh, *myGraphicsSystem) };
        N_CORE_ASSERT(ok, "failed to create fallback mesh");
        myGraphicsSystem->FlushUploads();
        myFallbackMesh.state = AssetState::Ready;
    
        NL_INFO(GCoreLogger, "fallback mesh created");
    }

    void AssetManager::InitializeFallbackTexture()
    {
        // 4x4 magenta/black checkerboard — immediately obvious in viewport
        constexpr uint32_t W{4};
        constexpr uint32_t H{4};

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
        raw.width     = W;
        raw.height    = H;
        raw.mipLevels = 1;
        raw.format    = Graphics::TextureFormat::RGBA8_UNORM;
        raw.mips.push_back({ .rowPitch = W * 4, .slicePitch = W * H * 4, .pixels = std::move(pixels) });

        const bool ok{ TextureProcessor::Process(raw, myFallbackTexture, *myGraphicsSystem) };
        N_CORE_ASSERT(ok, "failed to create fallback texture");
        myGraphicsSystem->FlushUploads();
        myFallbackTexture.state = AssetState::Ready;

        NL_INFO(GCoreLogger, "fallback texture created");
    }

    void AssetManager::InitializeFallbackPipeline()
    {
        const std::filesystem::path shaderPath{ Paths::EngineAssetDir() / "Shaders" / "Fallback.hlsl" };

        Graphics::ShaderDesc shaderDesc;
        shaderDesc.filePath = shaderPath;
        shaderDesc.stages   =
        {
            { Graphics::ShaderStage::Vertex, "VSMain" },
            { Graphics::ShaderStage::Pixel,  "PSMain" }
        };

        const auto shader{ myGraphicsSystem->GetShaderCompiler()->Compile(shaderDesc) };
        N_CORE_ASSERT(shader, "failed to compile fallback shader");

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = shader;
        pipelineDesc.rasterizer.cullMode = Graphics::CullMode::Back;
        pipelineDesc.rasterizer.fillMode = Graphics::FillMode::Solid;
        pipelineDesc.depth.depthEnabled  = true;
        pipelineDesc.depth.depthWrite    = true;
        pipelineDesc.depth.compareFunc   = Graphics::DepthCompare::Greater;
        pipelineDesc.blend.blendEnabled  = false;

        myFallbackPipeline.gpuHandle = myGraphicsSystem->CreatePipeline(pipelineDesc);
        N_CORE_ASSERT(myFallbackPipeline.gpuHandle.IsValid(), "failed to create fallback pipeline");
        myFallbackPipeline.state = AssetState::Ready;

        NL_INFO(GCoreLogger, "fallback pipeline created");
    }
}
