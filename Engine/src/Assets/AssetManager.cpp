#include "npch.h"
#include "Nalta/Assets/AssetManager.h"

#include "Nalta/Assets/Importers/DDSImporter.h"
#include "Nalta/Assets/Importers/ObjImporter.h"
#include "Nalta/Assets/Importers/TextureDescriptorImporter.h"
#include "Nalta/Assets/Importers/TextureImporter.h"
#include "Nalta/Assets/TypeHandlers/MeshTypeHandler.h"
#include "Nalta/Assets/TypeHandlers/TextureTypeHandler.h"
#include "Nalta/Core/BinaryIO.h"
#include "Nalta/Platform/IPlatformSystem.h"

namespace Nalta
{
    AssetManager::AssetManager() = default;
    AssetManager::~AssetManager() = default;

    void AssetManager::Initialize(Graphics::GPUResourceManager* aGraphicsSystem, IFileWatcher* aFileWatcher, IPlatformSystem* aPlatformSystem)
    {
        NL_SCOPE_CORE("AssetManager");
        N_CORE_ASSERT(aGraphicsSystem, "AssetManager: null graphics system");
        N_CORE_ASSERT(aFileWatcher, "AssetManager: null file watcher");

        myGPUResourceManager = aGraphicsSystem;
        myFileWatcher = aFileWatcher;

        myImporterRegistry.Register(std::make_unique<ObjImporter>());
        myImporterRegistry.Register(std::make_unique<TextureDescriptorImporter>());
        myImporterRegistry.Register(std::make_unique<TextureImporter>(".png"));
        myImporterRegistry.Register(std::make_unique<TextureImporter>(".jpg"));
        myImporterRegistry.Register(std::make_unique<TextureImporter>(".jpeg"));
        myImporterRegistry.Register(std::make_unique<TextureImporter>(".tga"));
        myImporterRegistry.Register(std::make_unique<DDSImporter>());

        // Register handlers - index must match AssetType enum value
        myHandlers.resize(2);
        myHandlers[static_cast<size_t>(AssetType::Mesh)] = std::make_unique<MeshTypeHandler>(myMeshStore, myGPUResourceManager, myImporterRegistry);
        myHandlers[static_cast<size_t>(AssetType::Texture)] = std::make_unique<TextureTypeHandler>(myTextureStore, myGPUResourceManager, myImporterRegistry);

        for (auto& handler : myHandlers)
        {
            if (handler)
            {
                handler->InitializeFallback();
            }
        }

        myRegistry.Initialize(Paths::CookedDir() / "AssetRegistry.json");

        myStop = false;
        myAssetThread = std::thread([this, aPlatformSystem]()
        {
            if (aPlatformSystem)
            {
                aPlatformSystem->SetCurrentThreadName("Asset");
            }
            
            AssetThreadLoop();
        });

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

        {
            std::lock_guard lock{ myPendingUploadsMutex };
            myPendingUploads.clear();
        }

        for (auto& handler : myHandlers)
        {
            if (handler)
            {
                handler->DestroyAll(*myGPUResourceManager);
            }
        }

        myRegistry.Shutdown();
        NL_INFO(GCoreLogger, "shutdown");
    }
    
    MeshKey AssetManager::RequestMesh(const AssetPath& aPath)
    {
        return RequestInternal(aPath, AssetType::Mesh, myMeshStore, false);
    }

    TextureKey AssetManager::RequestTexture(const AssetPath& aPath)
    {
        return RequestInternal(aPath, AssetType::Texture, myTextureStore, false);
    }

    const Mesh* AssetManager::GetMesh(const MeshKey aKey) const
    {
        return myMeshStore.GetReady(aKey);
    }

    const Texture* AssetManager::GetTexture(const TextureKey aKey) const
    {
        return myTextureStore.GetReady(aKey);
    }
    
    void AssetManager::OnFileChanged(const std::filesystem::path& aPath)
    {
        const AssetPath assetPath{ aPath };
        NL_INFO(GCoreLogger, "file changed: '{}'", assetPath.GetPath());

        DebounceReload(assetPath.GetPath());

        for (const auto& dependent : myRegistry.FindDependents(assetPath))
        {
            NL_INFO(GCoreLogger, "reloading dependent: '{}'", dependent);
            DebounceReload(dependent);
        }
    }

    void AssetManager::AssetThreadLoop()
    {
        NL_SCOPE_CORE("AssetThread");
        NL_INFO(GCoreLogger, "asset thread started");

        while (!myStop)
        {
            PollPendingUploads();

            // Fire any debounced reloads whose delay has elapsed
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
        IAssetTypeHandler& handler{ GetHandler(aRequest.type) };

        const auto cookedPath{ GetCookedPath(aRequest.path) };
        bool success{ false };

#ifndef N_SHIPPING
        if (!myRegistry.NeedsRecook(aRequest.path) && std::filesystem::exists(cookedPath))
        {
            success = handler.LoadFromCooked(aRequest.hash, aRequest.path, cookedPath);
            if (!success)
            {
                NL_WARN(GCoreLogger, "cooked load failed — falling back to full import");
            }
        }
        if (!success)
        {
            success = handler.CookAndProcess(aRequest.hash, aRequest.path, myRegistry);
        }
#else
        success = handler.LoadFromCooked(aRequest.hash, cookedPath);
#endif

        if (success)
        {
            std::lock_guard lock{ myPendingUploadsMutex };
            myPendingUploads.push_back({ aRequest.hash, aRequest.type, aRequest.isReload });
            return;
        }

        // Load failed — check fresh state to decide whether to preserve or fail
        const AssetState currentState{ handler.GetState(aRequest.hash) };
        if (aRequest.isReload && currentState == AssetState::Ready)
        {
            NL_WARN(GCoreLogger, "reload failed — keeping previous version of '{}'", aRequest.path.GetPath());
            // Leave state untouched - asset remains visible as Ready
        }
        else
        {
            NL_ERROR(GCoreLogger, "failed to load '{}'", aRequest.path.GetPath());
            handler.SetFailed(aRequest.hash);
        }
    }

    void AssetManager::PollPendingUploads()
    {
        std::lock_guard lock{ myPendingUploadsMutex };

        for (auto it{ myPendingUploads.begin() }; it != myPendingUploads.end();)
        {
            IAssetTypeHandler& handler{ GetHandler(it->type) };

            if (handler.IsUploadComplete(it->hash))
            {
                handler.SetReady(it->hash);
                NL_TRACE(GCoreLogger, "{} upload complete", it->type == AssetType::Mesh ? "mesh" : "texture");
                it = myPendingUploads.erase(it);
            }
            else
            {
                ++it;
            }
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
        const uint64_t hash{ path.GetHash() };

        // Walk handlers to find which type owns this hash
        for (size_t i{ 0 }; i < myHandlers.size(); ++i)
        {
            if (!myHandlers[i] || !myHandlers[i]->OwnsHash(hash))
            {
                continue;
            }

            // Delete stale cooked file so the next load re-cooks
            const auto cookedPath{ GetCookedPath(path) };
            if (std::filesystem::exists(cookedPath))
            {
                std::filesystem::remove(cookedPath);
            }

            {
                std::lock_guard lock{ myQueueMutex };
                myLoadQueue.push({ hash, path, static_cast<AssetType>(i), true });
            }
            myQueueCV.notify_one();
            return;
        }

        NL_TRACE(GCoreLogger, "file changed but '{}' is not loaded — skipping reload", aSourcePath);
    }

    IAssetTypeHandler& AssetManager::GetHandler(AssetType aType)
    {
        const auto idx{ static_cast<size_t>(aType) };
        N_CORE_ASSERT(idx < myHandlers.size() && myHandlers[idx], "no handler registered for asset type");
        return *myHandlers[idx];
    }

    const IAssetTypeHandler& AssetManager::GetHandler(AssetType aType) const
    {
        const auto idx{ static_cast<size_t>(aType) };
        N_CORE_ASSERT(idx < myHandlers.size() && myHandlers[idx], "no handler registered for asset type");
        return *myHandlers[idx];
    }

    std::filesystem::path AssetManager::GetCookedPath(const AssetPath& aPath)
    {
        const std::string filename{ std::format("{:016x}.nasset", aPath.GetHash()) };
        return Paths::CookedDir() / filename;
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

    bool AssetManager::ReadCookedHeader(BinaryReader& aReader, AssetType& aOutType)
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
}
