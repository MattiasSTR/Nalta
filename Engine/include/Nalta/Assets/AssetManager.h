#pragma once
#include "AssetRegistry.h"
#include "Nalta/Core/Assert.h"
#include "Nalta/Assets/AssetHandle.h"
#include "Nalta/Assets/AssetPath.h"
#include "Nalta/Assets/AssetRequest.h"
#include "Nalta/Assets/Importers/ImporterRegistry.h"
#include "Processors/ProcessorRegistry.h"
#include "Serializers/SerializerRegistry.h"

#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <chrono>

namespace Nalta
{
    class IFileWatcher;
    class GraphicsSystem;
    class IPlatformSystem;

    class AssetManager
    {
    public:
        AssetManager();
        ~AssetManager();

        void Initialize(GraphicsSystem* aGraphicsSystem, IPlatformSystem* aPlatformSystem);
        void Shutdown();

        template<typename T>
        [[nodiscard]] AssetRequest Request(const AssetPath& aPath)
        {
            std::shared_ptr<Asset> asset{ RequestInternal<T>(aPath) };
            return AssetRequest(asset);
        }

        template<typename T>
        [[nodiscard]] AssetHandle<T> RequestSync(const AssetPath& aPath)
        {
            AssetRequest request{ Request<T>(aPath) };
            while (!request.IsComplete())
            {
                std::this_thread::yield();
            }
            return request.GetHandle<T>();
        }

        [[nodiscard]] bool IsLoaded(const AssetPath& aPath) const;

    private:
        struct LoadRequest
        {
            std::shared_ptr<Asset> asset;
            AssetPath path;
            bool isReload{ false }; // keep old state on failure
            bool wasReady{ false };
        };

        template<typename T>
        std::shared_ptr<Asset> RequestInternal(const AssetPath& aPath)
        {
            N_CORE_ASSERT(!aPath.IsEmpty(), "empty asset path");

            {
                std::lock_guard lock{ myAssetsMutex };
                const auto it{ myAssets.find(aPath.GetHash()) };
                if (it != myAssets.end())
                {
                    return it->second;
                }
            }

            // Create asset and queue for loading
            auto asset{ std::make_shared<T>(aPath) };
            asset->SetState(AssetState::Requested);

            {
                std::lock_guard lock{ myAssetsMutex };
                myAssets[aPath.GetHash()] = asset;
            }

            {
                std::lock_guard lock{ myQueueMutex };
                myLoadQueue.push({ asset, aPath });
            }
            myQueueCV.notify_one();

            NL_TRACE(GCoreLogger, "requested '{}'", aPath.GetPath());
            return asset;
        }
        
        void AssetThreadLoop();
        void ProcessLoadRequest(const LoadRequest& aRequest);
        bool LoadFromCooked(const LoadRequest& aRequest, const std::filesystem::path& aCookedPath) const;
        void OnFileChanged(const std::filesystem::path& aPath);
        void ReloadAsset(const std::string& aSourcePath);
        void QueueReload(const std::string& aSourcePath);

        GraphicsSystem* myGraphicsSystem{ nullptr };
        std::unique_ptr<IFileWatcher> myFileWatcher;
        ImporterRegistry myImporterRegistry;
        SerializerRegistry mySerializerRegistry;
        AssetRegistry myRegistry;
        ProcessorRegistry myProcessorRegistry;

        mutable std::mutex myAssetsMutex;
        std::unordered_map<uint64_t, std::shared_ptr<Asset>> myAssets;

        std::mutex myQueueMutex;
        std::condition_variable myQueueCV;
        std::queue<LoadRequest> myLoadQueue;

        std::thread myAssetThread;
        std::atomic<bool> myStop{ false };
        
        struct DelayedReload
        {
            std::string sourcePath;
            std::chrono::steady_clock::time_point fireAt;
        };

        std::vector<DelayedReload> myDelayedReloads;
        std::mutex myDelayedReloadsMutex;
    };
}