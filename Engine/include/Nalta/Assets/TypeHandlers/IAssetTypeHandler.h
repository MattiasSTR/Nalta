#pragma once
#include "Nalta/Assets/AssetPath.h"
#include "Nalta/Assets/AssetRegistry.h"
#include "Nalta/Assets/AssetState.h"

#include <filesystem>


namespace Nalta
{
    namespace Graphics
    {
        class GPUResourceManager;
    }
    
    // Type-erased interface for one asset type's load/cook/upload/fallback logic.
    // AssetManager calls these; the handler knows nothing about queues or threading.
    class IAssetTypeHandler
    {
    public:
        virtual ~IAssetTypeHandler() = default;

        // Import source > cook binary > kick GPU upload. Returns false on any failure.
        virtual bool CookAndProcess(uint64_t aHash, const AssetPath& aPath, AssetRegistry& aRegistry) = 0;

        // Deserialize existing cooked binary > kick GPU upload.
        virtual bool LoadFromCooked(uint64_t aHash, const AssetPath& aSourcePath, const std::filesystem::path& aCookedPath) = 0;

        // Returns true once the GPU upload initiated by Cook/Load has landed.
        virtual bool IsUploadComplete(uint64_t aHash) const = 0;

        // Mark the asset Ready (called by manager when IsUploadComplete flips true).
        virtual void SetReady(uint64_t aHash) = 0;

        // Mark the asset Failed.
        virtual void SetFailed(uint64_t aHash) = 0;

        // Re-expose the asset's current state (for wasReady check at failure time).
        virtual AssetState GetState(uint64_t aHash) const = 0;
        
        virtual bool OwnsHash(uint64_t aHash) const = 0;

        // Upload complete > transition to Ready and run any post-ready work.
        virtual void InitializeFallback() = 0;

        // Release all GPU resources for every live asset of this type.
        virtual void DestroyAll(Graphics::GPUResourceManager& aGPU) = 0;
    };
}