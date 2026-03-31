#pragma once
#include "Nalta/Assets/AssetKeys.h"
#include "Nalta/Assets/AssetPath.h"
#include "Nalta/Assets/AssetRegistry.h"
#include "Nalta/Assets/AssetState.h"
#include "Nalta/Assets/Mesh.h"
#include "Nalta/Assets/Pipeline.h"
#include "Nalta/Assets/Texture.h"
#include "Nalta/Assets/Importers/ImporterRegistry.h"
#include "Nalta/Util/SlotMap.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

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
        [[nodiscard]] MeshKey RequestMesh(const AssetPath& aPath);
        [[nodiscard]] TextureKey RequestTexture(const AssetPath& aPath);
        [[nodiscard]] PipelineKey RequestPipeline(const AssetPath& aPath);

        // Get loaded asset data - returns fallback if not ready yet
        [[nodiscard]] const Mesh* GetMesh(MeshKey aKey) const;
        [[nodiscard]] const Texture* GetTexture(TextureKey aKey) const;
        [[nodiscard]] const Pipeline* GetPipeline(PipelineKey aKey) const;

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
        MeshKey RequestMeshInternal(const AssetPath& aPath, bool aIsReload);
        TextureKey RequestTextureInternal(const AssetPath& aPath, bool aIsReload);
        PipelineKey RequestPipelineInternal(const AssetPath& aPath, bool aIsReload);
        
        MeshKey GetMeshKey(uint64_t aHash) const;
        TextureKey GetTextureKey(uint64_t aHash) const;
        PipelineKey GetPipelineKey(uint64_t aHash) const;

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
        void SetMeshState(MeshKey aKey, AssetState aState, bool aIsReload);
        void SetTextureState(TextureKey aKey, AssetState aState, bool aIsReload);
        void SetPipelineState(PipelineKey aKey, AssetState aState, bool aIsReload);
        void PromotePendingAssets();
        static void WriteCookedHeader(BinaryWriter& aWriter, AssetType aType);
        static bool ReadCookedHeader(BinaryReader& aReader, AssetType& aOutType);
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

        SlotMap<MeshKey, Mesh> myMeshes;
        std::unordered_map<uint64_t, MeshKey> myMeshIndex;
        
        SlotMap<TextureKey, Texture> myTextures;
        std::unordered_map<uint64_t, TextureKey> myTextureIndex;
        
        SlotMap<PipelineKey, Pipeline> myPipelines;
        std::unordered_map<uint64_t, PipelineKey> myPipelineIndex;
        
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
