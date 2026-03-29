#pragma once
#include "Nalta/Assets/Processors/IAssetProcessor.h"

namespace Nalta
{
    class MeshProcessor final : public IAssetProcessor
    {
    public:
        [[nodiscard]] bool Process(const RawAssetData& aData,Asset& aOutAsset, GraphicsSystem& aGraphicsSystem) const override;
        [[nodiscard]] AssetType GetAssetType() const override { return AssetType::Mesh; }
    };
}