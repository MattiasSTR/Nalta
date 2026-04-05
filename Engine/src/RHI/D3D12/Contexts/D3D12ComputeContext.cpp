#include "npch.h"
#include "Nalta/RHI/D3D12/Contexts/D3D12ComputeContext.h"
#include "Nalta/RHI/D3D12/D3D12Device.h"
#include "Nalta/RHI/D3D12/Resources/D3D12PipelineStateObject.h"

namespace Nalta::RHI::D3D12
{
    ComputeContext::ComputeContext(Device& aDevice)
        : Context(aDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
    }

    void ComputeContext::Reset()
    {
        Context::Reset();
        myCommandList->SetComputeRootSignature(myDevice.GetRootSignature());
    }
    
    void ComputeContext::SetPipeline(const PipelineStateObject& aPipeline)
    {
        N_CORE_ASSERT(aPipeline.IsValid(), "Setting invalid pipeline");
        N_CORE_ASSERT(aPipeline.isCompute, "Use GraphicsContext::SetPipeline for graphics pipelines");
        myCommandList->SetPipelineState(aPipeline.pipelineState);
    }

    void ComputeContext::SetRootConstants(const void* aData, const uint32_t aCount, const uint32_t aOffset)
    {
        N_CORE_ASSERT(aOffset + aCount <= ROOT_CONSTANT_COUNT, "Root constants out of range");
        myCommandList->SetComputeRoot32BitConstants(
            static_cast<uint32_t>(RootParameter::Constants),
            aCount,
            aData,
            aOffset);
    }

    void ComputeContext::SetPassCBV(const uint64_t aGPUAddress)
    {
        myCommandList->SetComputeRootConstantBufferView(static_cast<uint32_t>(RootParameter::PassCBV), aGPUAddress);
    }
    
    void ComputeContext::Dispatch(const uint32_t aGroupX, const uint32_t aGroupY, const uint32_t aGroupZ)
    {
        myCommandList->Dispatch(aGroupX, aGroupY, aGroupZ);
    }

    void ComputeContext::Dispatch1D(const uint32_t aThreadCountX, const uint32_t aGroupSizeX)
    {
        Dispatch((aThreadCountX + aGroupSizeX - 1) / aGroupSizeX, 1, 1);
    }

    void ComputeContext::Dispatch2D(const uint32_t aThreadCountX, const uint32_t aThreadCountY, const uint32_t aGroupSizeX, const uint32_t aGroupSizeY)
    {
        Dispatch((aThreadCountX + aGroupSizeX - 1) / aGroupSizeX, (aThreadCountY + aGroupSizeY - 1) / aGroupSizeY, 1);
    }
    
    void ComputeContext::Dispatch3D(const uint32_t aThreadCountX, const uint32_t aThreadCountY, const uint32_t aThreadCountZ, const uint32_t aGroupSizeX, const uint32_t aGroupSizeY, const uint32_t aGroupSizeZ)
    {
        Dispatch((aThreadCountX + aGroupSizeX - 1) / aGroupSizeX, (aThreadCountY + aGroupSizeY - 1) / aGroupSizeY, (aThreadCountZ + aGroupSizeZ - 1) / aGroupSizeZ);
    }
}
