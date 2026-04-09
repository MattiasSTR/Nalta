#pragma once
#include "AssetStore.h"
#include "AssetKeys.h"
#include "AssetPath.h"
#include "AssetRegistry.h"
#include "Mesh.h"
#include "Texture.h"
#include "Importers/ImporterRegistry.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <queue>
#include <thread>

namespace Nalta
{
    class IAssetTypeHandler;
    class MeshTypeHandler;

    namespace Graphics
    {
        class GPUResourceManager;
    }
    
    class BinaryReader;
    class BinaryWriter;
    class IPlatformSystem;
    class IFileWatcher;
    
    enum class AssetType : uint8_t
    {
        Mesh, 
        Texture
    };

    class AssetManager
    {
    public:
        AssetManager();
        ~AssetManager();

        void Initialize(Graphics::GPUResourceManager* aGraphicsSystem, IFileWatcher* aFileWatcher, IPlatformSystem* aPlatformSystem);
        void Shutdown();

        // Request an asset load - returns a handle immediately, load happens async
        [[nodiscard]] MeshKey RequestMesh(const AssetPath& aPath);
        [[nodiscard]] TextureKey RequestTexture(const AssetPath& aPath);

        // Get loaded asset data - returns fallback if not ready yet
        [[nodiscard]] const Mesh* GetMesh(MeshKey aKey) const;
        [[nodiscard]] const Texture* GetTexture(TextureKey aKey) const;

        static std::filesystem::path GetCookedPath(const AssetPath& aPath);
        static void WriteCookedHeader(BinaryWriter& aWriter, AssetType aType);
        static bool ReadCookedHeader(BinaryReader& aReader, AssetType& aOutType);

    private:
        struct LoadRequest
        {
            uint64_t hash;
            AssetPath path;
            AssetType type;
            bool isReload{ false };
        };

        struct DelayedReload
        {
            std::string sourcePath;
            std::chrono::steady_clock::time_point fireAt;
        };
        
        void OnFileChanged(const std::filesystem::path& aPath);

        template<class TKey, class TAsset>
        TKey RequestInternal(const AssetPath& aPath, AssetType aType, AssetStore<TKey, TAsset>& aStore, bool aIsReload);

        // Asset thread
        void AssetThreadLoop();
        void ProcessLoadRequest(const LoadRequest&);
        void PollPendingUploads();

        // Hot reload
        void DebounceReload(const std::string&);
        void QueueReload(const std::string&);
        
        IAssetTypeHandler& GetHandler(AssetType aType);
        const IAssetTypeHandler& GetHandler(AssetType aType) const;
        
        // Per-type stores (owned here, handlers hold references)
        AssetStore<MeshKey, Mesh> myMeshStore;
        AssetStore<TextureKey, Texture> myTextureStore;

        std::vector<std::unique_ptr<IAssetTypeHandler>> myHandlers;

        // Pending uploads: just (hash, type) - handler knows how to check
        struct PendingUpload { uint64_t hash; AssetType type; bool isReload; };
        std::vector<PendingUpload> myPendingUploads;
        std::mutex myPendingUploadsMutex;

        // Async machinery
        std::mutex myQueueMutex;
        std::condition_variable myQueueCV;
        std::queue<LoadRequest> myLoadQueue;
        std::thread myAssetThread;
        std::atomic<bool> myStop{ false };

        std::mutex myDelayedReloadsMutex;
        std::vector<DelayedReload> myDelayedReloads;

        ImporterRegistry myImporterRegistry;
        AssetRegistry myRegistry;
        Graphics::GPUResourceManager* myGPUResourceManager{ nullptr };
    };

    template<typename TKey, typename TAsset>
    TKey AssetManager::RequestInternal(const AssetPath& aPath, const AssetType aType,AssetStore<TKey, TAsset>& aStore, const bool aIsReload)
    {
        N_CORE_ASSERT(!aPath.IsEmpty(), "empty asset path");

        const uint64_t hash{ aPath.GetHash() };
        const auto [key, inserted]{ aStore.GetOrInsert(hash) };

        if (!inserted && !aIsReload)
        {
            return key;
        }

        // Both new insertions and reloads need state reset to Requested
        aStore.SetState(key, AssetState::Requested);

        {
            std::lock_guard lock{ myQueueMutex };
            myLoadQueue.push({ hash, aPath, aType, aIsReload });
        }
        myQueueCV.notify_one();

        NL_TRACE(GCoreLogger, "requested {} '{}'", aType == AssetType::Mesh ? "mesh" : "texture", aPath.GetPath());
        return key;
    }
}
