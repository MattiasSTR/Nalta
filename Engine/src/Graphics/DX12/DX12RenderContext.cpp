#include "npch.h"
#include "Nalta/Graphics/DX12/DX12RenderContext.h"
#include "Nalta/Graphics/DX12/DX12Device.h"
#include "Nalta/Graphics/DX12/DX12Pipeline.h"
#include "Nalta/Graphics/RenderCommands.h"

#include <d3d12.h>

namespace Nalta::Graphics
{
    DX12RenderContext::DX12RenderContext(DX12Device* aDevice)
        : myDevice(aDevice)
    {}

    DX12RenderContext::~DX12RenderContext() = default;
    
    void DX12RenderContext::Begin()
    {
        // Nothing needed here yet — BeginFrame on the device handles
        // command allocator reset and command list reset
    }
    
    void DX12RenderContext::Execute(const RenderFrame& aFrame)
    {
        auto* cmdList{ myDevice->GetCommandList() };

        for (const auto& command : aFrame.commands)
        {
            std::visit([&]<typename T0>(const T0& aCmd)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, SetPipelineCmd>)
                {
                    N_ASSERT(aCmd.pipeline.IsValid(), "DX12RenderContext: invalid pipeline handle");
                    const auto* pipeline{ static_cast<DX12Pipeline*>(aCmd.pipeline.Get()) };
                    cmdList->SetGraphicsRootSignature(pipeline->GetRootSignature());
                    cmdList->SetPipelineState(pipeline->GetPipelineState());
                    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                }
                else if constexpr (std::is_same_v<T, DrawCmd>)
                {
                    cmdList->DrawInstanced(
                        aCmd.vertexCount,
                        aCmd.instanceCount,
                        aCmd.vertexOffset,
                        aCmd.instanceOffset);
                }
                else if constexpr (std::is_same_v<T, DrawIndexedCmd>)
                {
                    cmdList->DrawIndexedInstanced(
                        aCmd.indexCount,
                        aCmd.instanceCount,
                        aCmd.indexOffset,
                        aCmd.vertexOffset,
                        aCmd.instanceOffset);
                }
            }, command);
        }
    }
}