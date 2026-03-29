#pragma once
#include "Nalta/Assets/AssetType.h"

namespace Nalta
{
    struct RawAssetData;
    class Asset;
    class GraphicsSystem;

    class IAssetProcessor
    {
    public:
        virtual ~IAssetProcessor() = default;
        
        [[nodiscard]] virtual bool Process(const RawAssetData& aData, Asset& aOutAsset, GraphicsSystem& aGraphicsSystem) const = 0;
        [[nodiscard]] virtual AssetType GetAssetType() const = 0;
    };
}