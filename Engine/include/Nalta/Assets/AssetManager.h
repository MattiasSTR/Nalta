#pragma once
#include "Nalta/Assets/AssetHandle.h"
#include "Nalta/Assets/AssetRegistry.h"
#include "Nalta/Assets/AssetPath.h"
#include "Nalta/Assets/AssetState.h"
#include "Nalta/Assets/Importers/ImporterRegistry.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <chrono>

namespace Nalta
{
    class BinaryReader;
    class BinaryWriter;
    class GraphicsSystem;
    class IPlatformSystem;
    class IFileWatcher;

    class AssetManager
    {
    public:
        AssetManager();
        ~AssetManager();

        void Initialize(GraphicsSystem* aGraphicsSystem, IPlatformSystem* aPlatformSystem);
        void Shutdown();

        // Request an asset load - returns a handle immediately, load happens async
        [[nodiscard]] MeshHandle RequestMesh(const AssetPath& aPath);
        [[nodiscard]] TextureHandle RequestTexture(const AssetPath& aPath);
        [[nodiscard]] PipelineHandle RequestPipeline(const AssetPath& aPath);

        // Get loaded asset data - returns fallback if not ready yet
        [[nodiscard]] const Mesh* GetMesh(MeshHandle aHandle) const;
        [[nodiscard]] const Texture* GetTexture(TextureHandle aHandle) const;
        [[nodiscard]] const Pipeline* GetPipeline(PipelineHandle aHandle) const;

    private:
        enum class AssetType : uint8_t
        {
            Mesh, 
            Texture, 
            Pipeline
        };

        struct LoadRequest
        {
            uint64_t id;
            AssetPath path;
            AssetType type;
            bool isReload{ false };
            bool wasReady{ false };
        };

        struct DelayedReload
        {
            std::string sourcePath;
            std::chrono::steady_clock::time_point fireAt;
        };

        // Internal request helpers
        MeshHandle RequestMeshInternal(const AssetPath& aPath, bool aIsReload);
        TextureHandle RequestTextureInternal(const AssetPath& aPath, bool aIsReload);
        PipelineHandle RequestPipelineInternal(const AssetPath& aPath, bool aIsReload);

        // Asset thread
        void AssetThreadLoop();
        void ProcessLoadRequest(const LoadRequest& aRequest);

        // Per-type load paths
        bool LoadMesh(const LoadRequest& aRequest);
        bool LoadTexture(const LoadRequest& aRequest);
        bool LoadPipeline(const LoadRequest& aRequest);

        // Cook helpers
        bool CookAndProcessMesh(const LoadRequest& aRequest, const AssetPath& aPath);
        bool CookAndProcessTexture(const LoadRequest& aRequest, const AssetPath& aPath);
        bool CookAndProcessPipeline(const LoadRequest& aRequest, const AssetPath& aPath);

        bool LoadMeshFromCooked(const LoadRequest& aRequest, const std::filesystem::path& aCookedPath);
        bool LoadTextureFromCooked(const LoadRequest& aRequest, const std::filesystem::path& aCookedPath);
        bool LoadPipelineFromCooked(const LoadRequest& aRequest, const std::filesystem::path& aCookedPath);

        // Hot reload
        void OnFileChanged(const std::filesystem::path& aPath);
        void DebounceReload(const std::string& aSourcePath);
        void QueueReload(const std::string& aSourcePath);

        // Utilities
        [[nodiscard]] static std::filesystem::path GetCookedPath(const AssetPath& aPath);
        void SetMeshState(uint64_t aId, AssetState aState, bool aIsReload);
        void SetTextureState(uint64_t aId, AssetState aState, bool aIsReload);
        void SetPipelineState(uint64_t aId, AssetState aState, bool aIsReload);
        void PromotePendingAssets();
        static void WriteCookedHeader(BinaryWriter& aWriter, AssetType aType);
        bool ReadCookedHeader(BinaryReader& aReader, AssetType& aOutType) const;
        void RegisterCookedEntry(const AssetPath& aPath, const std::filesystem::path& aCookedPath, AssetType aType, const std::vector<std::string>& aDependencies);
        
        // Fallbacks
        void InitializeFallbacks();
        void InitializeFallbackMesh();
        void InitializeFallbackTexture();
        void InitializeFallbackPipeline();

        // Storage - one map per type, one mutex per type
        mutable std::mutex myMeshMutex;
        mutable std::mutex myTextureMutex;
        mutable std::mutex myPipelineMutex;

        std::unordered_map<uint64_t, Mesh> myMeshes;
        std::unordered_map<uint64_t, Texture> myTextures;
        std::unordered_map<uint64_t, Pipeline> myPipelines;
        
        // Fallbacks
        Mesh myFallbackMesh;
        Texture myFallbackTexture;
        Pipeline myFallbackPipeline;

        // Pipeline
        ImporterRegistry myImporterRegistry;
        AssetRegistry myRegistry;

        // Async machinery
        std::mutex myQueueMutex;
        std::condition_variable myQueueCV;
        std::queue<LoadRequest> myLoadQueue;
        std::thread myAssetThread;
        std::atomic<bool> myStop{ false };

        std::mutex myDelayedReloadsMutex;
        std::vector<DelayedReload> myDelayedReloads;

        // Systems
        GraphicsSystem* myGraphicsSystem{ nullptr };
        std::unique_ptr<IFileWatcher> myFileWatcher;
    };
}