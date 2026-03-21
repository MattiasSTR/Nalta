#include "npch.h"
#include "Nalta/Graphics/DX12/DX12Pipeline.h"

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Nalta::Graphics
{
    struct DX12Pipeline::Impl
    {
        ComPtr<ID3D12PipelineState> pipelineState;
        ComPtr<ID3D12RootSignature> rootSignature;
    };

    DX12Pipeline::DX12Pipeline(ID3D12PipelineState* aPipelineState, ID3D12RootSignature* aRootSignature)
        : myImpl(std::make_unique<Impl>())
    {
        myImpl->pipelineState = aPipelineState;
        myImpl->rootSignature = aRootSignature;
    }

    DX12Pipeline::~DX12Pipeline() = default;

    bool DX12Pipeline::IsValid() const
    {
        return myImpl->pipelineState != nullptr && myImpl->rootSignature != nullptr;
    }

    ID3D12PipelineState* DX12Pipeline::GetPipelineState() const { return myImpl->pipelineState.Get(); }
    ID3D12RootSignature* DX12Pipeline::GetRootSignature() const { return myImpl->rootSignature.Get(); }
}