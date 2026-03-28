#include "npch.h"
#include "Nalta/Assets/AssetManager.h"
#include "Nalta/Assets/Asset.h"
#include "Nalta/Assets/Importers/IAssetImporter.h"
#include "Nalta/Assets/Importers/ObjImporter.h"
#include "Nalta/Assets/Mesh/MeshAsset.h"
#include "Nalta/Assets/Processors/RawAssetData.h"
#include "Nalta/Graphics/GraphicsSystem.h"
#include "Nalta/Platform/IPlatformSystem.h"

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
        myMeshProcessor = MeshProcessor(aGraphicsSystem);
        
        myImporterRegistry.Register(std::make_unique<ObjImporter>());

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

        // Wait for GPU to finish before releasing assets
        if (myGraphicsSystem != nullptr)
        {
            myGraphicsSystem->WaitForGPU();
        }

        {
            std::lock_guard lock{ myAssetsMutex };
            myAssets.clear();
        }

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
            LoadRequest request;

            {
                std::unique_lock lock{ myQueueMutex };
                myQueueCV.wait(lock, [this]
                {
                    return !myLoadQueue.empty() || myStop;
                });

                if (myStop && myLoadQueue.empty())
                {
                    break;
                }

                request = myLoadQueue.front();
                myLoadQueue.pop();
            }

            ProcessLoadRequest(request);
        }

        NL_INFO(GCoreLogger, "stopped");
    }

    void AssetManager::ProcessLoadRequest(const LoadRequest& aRequest) const
    {
        NL_SCOPE_CORE("AssetManager");
        NL_TRACE(GCoreLogger, "processing '{}'", aRequest.path.GetPath());

        aRequest.asset->SetState(AssetState::Loading);

        IAssetImporter* importer{ myImporterRegistry.GetImporter(aRequest.path) };
        if (importer == nullptr)
        {
            NL_ERROR(GCoreLogger, "no importer for '{}'", aRequest.path.GetPath());
            aRequest.asset->SetState(AssetState::Failed);
            return;
        }

        aRequest.asset->SetState(AssetState::Processing);
        const auto rawData{ importer->Import(aRequest.path) };
        if (!rawData || !rawData->IsValid())
        {
            NL_ERROR(GCoreLogger, "import failed for '{}'", aRequest.path.GetPath());
            aRequest.asset->SetState(AssetState::Failed);
            return;
        }
        
        // Process based on asset type
        if (aRequest.asset->GetAssetType() == AssetType::Mesh)
        {
            auto* meshAsset{ static_cast<MeshAsset*>(aRequest.asset.get()) };
            aRequest.asset->SetState(AssetState::Uploading);
            if (!myMeshProcessor.Process(*rawData, *meshAsset))
            {
                aRequest.asset->SetState(AssetState::Failed);
                return;
            }

            // Queue GPU upload and flush
            myGraphicsSystem->FlushUploads();
        }

        aRequest.asset->SetState(AssetState::Ready);
        NL_INFO(GCoreLogger, "loaded '{}'", aRequest.path.GetPath());
    }
}
