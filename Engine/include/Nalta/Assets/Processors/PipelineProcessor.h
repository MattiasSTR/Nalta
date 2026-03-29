#pragma once
#include "Nalta/Assets/Processors/IAssetProcessor.h"

namespace Nalta
{
    class PipelineProcessor final : public IAssetProcessor
    {
    public:
        [[nodiscard]] bool Process(const RawAssetData& aData, Asset& aOutAsset, GraphicsSystem& aGraphicsSystem) const override;

        [[nodiscard]] AssetType GetAssetType() const override { return AssetType::Pipeline; }
    };
}