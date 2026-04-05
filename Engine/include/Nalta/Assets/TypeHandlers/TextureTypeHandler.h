#pragma once
#include "IAssetTypeHandler.h"
#include "Nalta/Assets/AssetKeys.h"
#include "Nalta/Assets/AssetStore.h"
#include "Nalta/Assets/Texture.h"
#include "Nalta/Assets/Importers/ImporterRegistry.h"

namespace Nalta
{
    class TextureTypeHandler final : public IAssetTypeHandler
    {
    public:
        TextureTypeHandler(AssetStore<TextureKey, Texture>& aStore, Graphics::GPUResourceManager* aGPU, ImporterRegistry& aImporters)
            : myStore(aStore), myGPU(aGPU), myImporters(aImporters) {}

        bool CookAndProcess(uint64_t aHash, const AssetPath& aPath, AssetRegistry& aRegistry) override;
        bool LoadFromCooked(uint64_t aHash, const AssetPath& aSourcePath, const std::filesystem::path& aCookedPath) override;
        bool IsUploadComplete(uint64_t aHash) const override;
        bool OwnsHash(uint64_t aHash) const override;
        void SetReady(uint64_t aHash) override;
        void SetFailed(uint64_t aHash) override;
        AssetState GetState(uint64_t aHash) const override;
        void InitializeFallback() override;
        void DestroyAll(Graphics::GPUResourceManager& aGPU) override;

    private:
        AssetStore<TextureKey, Texture>& myStore;
        Graphics::GPUResourceManager* myGPU;
        ImporterRegistry& myImporters;
        Texture myFallback;
    };
}