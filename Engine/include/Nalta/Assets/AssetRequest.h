#pragma once
#include "Nalta/Assets/AssetHandle.h"

#include <memory>

namespace Nalta
{
    class AssetRequest
    {
    public:
        AssetRequest() = default;
        explicit AssetRequest(std::weak_ptr<Asset> aAsset) : myAsset(std::move(aAsset)) {}

        [[nodiscard]] bool IsComplete() const
        {
            const auto asset{ myAsset.lock() };
            return asset && (asset->GetState() == AssetState::Ready || asset->GetState() == AssetState::Failed);
        }

        [[nodiscard]] bool IsReady()  const
        {
            const auto asset{ myAsset.lock() };
            return asset && asset->IsReady();
        }

        [[nodiscard]] bool IsFailed() const
        {
            const auto asset{ myAsset.lock() };
            return asset && asset->IsFailed();
        }

        [[nodiscard]] float GetProgress() const
        {
            const auto asset{ myAsset.lock() };
            if (!asset) return 0.0f;
            switch (asset->GetState())
            {
                case AssetState::Unloaded:   return 0.0f;
                case AssetState::Requested:  return 0.1f;
                case AssetState::Loading:    return 0.3f;
                case AssetState::Processing: return 0.6f;
                case AssetState::Uploading:  return 0.9f;
                case AssetState::Ready:      return 1.0f;
                case AssetState::Failed:     return 0.0f;
                default:                     return 0.0f;
            }
        }

        template<typename T>
        [[nodiscard]] AssetHandle<T> GetHandle() const
        {
            return AssetHandle<T>(myAsset);
        }

    private:
        std::weak_ptr<Asset> myAsset;
    };
}
