#pragma once
#include "Nalta/Assets/AssetPath.h"
#include "Nalta/Assets/AssetState.h"
#include "Nalta/Assets/AssetType.h"

namespace Nalta
{
    class Asset : public std::enable_shared_from_this<Asset>
    {
    public:
        explicit Asset(AssetPath aPath) : myPath(std::move(aPath)) {}
        virtual ~Asset() = default;

        Asset(const Asset&) = delete;
        Asset& operator=(const Asset&) = delete;
        Asset(Asset&&) = delete;
        Asset& operator=(Asset&&) = delete;

        [[nodiscard]] virtual AssetType GetAssetType() const = 0;
        [[nodiscard]] const AssetPath& GetPath()  const { return myPath; }
        [[nodiscard]] AssetState GetState() const { return myState.load(); }
        [[nodiscard]] bool IsReady() const { return myState == AssetState::Ready; }
        [[nodiscard]] bool IsFailed() const { return myState == AssetState::Failed; }

    protected:
        void SetState(const AssetState aState) { myState = aState; }

    private:
        friend class AssetManager; // AssetManager drives state transitions

        AssetPath myPath;
        std::atomic<AssetState> myState{ AssetState::Unloaded };
    };
}