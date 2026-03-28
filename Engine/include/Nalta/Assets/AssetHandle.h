#pragma once
#include "Nalta/Assets/Asset.h"

#include <memory>

namespace Nalta
{
    template<typename T>
    class AssetHandle
    {
        static_assert(std::is_base_of_v<Asset, T>, "AssetHandle<T> requires T to derive from Asset");

    public:
        AssetHandle() = default;
        explicit AssetHandle(std::weak_ptr<Asset> aAsset) : myAsset(std::move(aAsset)) {}
        explicit AssetHandle(const std::shared_ptr<Asset>& aAsset) : myAsset(aAsset) {}

        [[nodiscard]] bool IsValid() const { return !myAsset.expired(); }
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
        
        [[nodiscard]] std::shared_ptr<T> Get() const
        {
            return std::static_pointer_cast<T>(myAsset.lock());
        }

        explicit operator bool() const { return IsValid(); }

        bool operator==(const AssetHandle& aOther) const
        {
            return !myAsset.owner_before(aOther.myAsset) && !aOther.myAsset.owner_before(myAsset);
        }
        bool operator!=(const AssetHandle& aOther) const { return !(*this == aOther); }

        // Upcast to base
        operator AssetHandle<Asset>() const { return AssetHandle<Asset>(myAsset); }

    private:
        std::weak_ptr<Asset> myAsset;
    };
}