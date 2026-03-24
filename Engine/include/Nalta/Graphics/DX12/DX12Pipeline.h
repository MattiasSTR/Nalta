#pragma once
#include "Nalta/Graphics/Pipeline/IPipeline.h"

#include <memory>

struct ID3D12PipelineState;
struct ID3D12RootSignature;

namespace Nalta::Graphics
{
    class DX12Pipeline final : public IPipeline
    {
    public:
        DX12Pipeline(ID3D12PipelineState* aPipelineState, ID3D12RootSignature* aRootSignature);
        ~DX12Pipeline() override;

        [[nodiscard]] bool IsValid() const override;
        [[nodiscard]] ID3D12PipelineState* GetPipelineState() const;
        [[nodiscard]] ID3D12RootSignature* GetRootSignature() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> myImpl;
    };
}