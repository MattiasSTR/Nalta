#include "npch.h"
#include "Nalta/Graphics/DX12/DX12RenderContext.h"

#include "Nalta/Graphics/Commands/RenderCommands.h"
#include "Nalta/Graphics/DX12/DX12ConstantBuffer.h"
#include "Nalta/Graphics/DX12/DX12Device.h"
#include "Nalta/Graphics/DX12/DX12IndexBuffer.h"
#include "Nalta/Graphics/DX12/DX12Pipeline.h"
#include "Nalta/Graphics/DX12/DX12VertexBuffer.h"

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
        NL_SCOPE_CORE("DX12RenderContext");
        auto* cmdList{ myDevice->GetCommandList() };

        for (const auto& command : aFrame.commands)
        {
            std::visit([&]<typename T0>(const T0& aCmd)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, SetPipelineCmd>)
                {
                    N_CORE_ASSERT(aCmd.pipeline.IsValid(), "invalid pipeline handle");
                    const auto* pipeline{ static_cast<DX12Pipeline*>(aCmd.pipeline.Get()) };
                    cmdList->SetGraphicsRootSignature(pipeline->GetRootSignature());
                    cmdList->SetPipelineState(pipeline->GetPipelineState());
                    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                }
                else if constexpr (std::is_same_v<T, SetVertexBufferCmd>)
                {
                    N_ASSERT(aCmd.buffer.IsValid(), "invalid vertex buffer handle");
                    const auto* vb{ static_cast<DX12VertexBuffer*>(aCmd.buffer.Get()) };
                    N_ASSERT(vb->IsReady(), "vertex buffer not yet uploaded");

                    const D3D12_VERTEX_BUFFER_VIEW view
                    {
                        .BufferLocation = vb->GetGPUAddress(),
                        .SizeInBytes    = vb->GetSizeInBytes(),
                        .StrideInBytes  = vb->GetStride()
                    };

                    cmdList->IASetVertexBuffers(0, 1, &view);
                }
                else if constexpr (std::is_same_v<T, SetIndexBufferCmd>)
                {
                    N_ASSERT(aCmd.buffer.IsValid(), "invalid index buffer handle");
                    auto* ib{ static_cast<DX12IndexBuffer*>(aCmd.buffer.Get()) };
                    N_ASSERT(ib->IsReady(), "index buffer not yet uploaded");

                    const D3D12_INDEX_BUFFER_VIEW view
                    {
                        .BufferLocation = ib->GetGPUAddress(),
                        .SizeInBytes    = ib->GetSizeInBytes(),
                        .Format         = ib->GetFormat() == IndexFormat::Uint16
                            ? DXGI_FORMAT_R16_UINT
                            : DXGI_FORMAT_R32_UINT
                    };

                    cmdList->IASetIndexBuffer(&view);
                }
                else if constexpr (std::is_same_v<T, UpdateConstantBufferCmd>)
                {
                    N_ASSERT(aCmd.buffer.IsValid(), "invalid constant buffer handle");
                    auto* cb{ static_cast<DX12ConstantBuffer*>(aCmd.buffer.Get()) };
                    cb->Update(aCmd.data.data(), static_cast<uint32_t>(aCmd.data.size()));
                }
                else if constexpr (std::is_same_v<T, SetConstantBufferCmd>)
                {
                    N_ASSERT(aCmd.buffer.IsValid(), "invalid constant buffer handle");
                    auto* cb{ static_cast<DX12ConstantBuffer*>(aCmd.buffer.Get()) };
                    cmdList->SetGraphicsRootConstantBufferView(
                        aCmd.rootParameterIndex,
                        cb->GetGPUAddress());
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
