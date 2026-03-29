#pragma once
#include "Nalta/Assets/Asset.h"
#include "Nalta/Graphics/Pipeline/PipelineHandle.h"

namespace Nalta
{
    class PipelineAsset final : public Asset
    {
    public:
        explicit PipelineAsset(AssetPath aPath) : Asset(std::move(aPath)) {}
        ~PipelineAsset() override = default;

        [[nodiscard]] AssetType GetAssetType() const override { return AssetType::Pipeline; }
        [[nodiscard]] Graphics::PipelineHandle GetPipelineHandle() const { return myPipelineHandle; }

    private:
        friend class PipelineProcessor;
        Graphics::PipelineHandle myPipelineHandle;
    };
}