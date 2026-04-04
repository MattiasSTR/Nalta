#include "npch.h"
#include "Nalta/RHI/D3D12/D3D12GraphicsContext.h"
#include "Nalta/RHI/D3D12/D3D12Device.h"

namespace Nalta::RHI::D3D12
{
    GraphicsContext::GraphicsContext(Device& aDevice)
        : Context(aDevice, D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
    }
    
    void GraphicsContext::SetViewport(const uint32_t aWidth, const uint32_t aHeight)
    {
        D3D12_VIEWPORT vp{};
        vp.Width    = static_cast<float>(aWidth);
        vp.Height   = static_cast<float>(aHeight);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0.0f;
        vp.TopLeftY = 0.0f;
        myCommandList->RSSetViewports(1, &vp);
    }

    void GraphicsContext::SetViewport(const D3D12_VIEWPORT& aViewport)
    {
        myCommandList->RSSetViewports(1, &aViewport);
    }

    void GraphicsContext::SetScissor(const uint32_t aWidth, const uint32_t aHeight)
    {
        const D3D12_RECT rect{ 0, 0, static_cast<LONG>(aWidth), static_cast<LONG>(aHeight) };
        myCommandList->RSSetScissorRects(1, &rect);
    }
    
    void GraphicsContext::SetScissor(const D3D12_RECT& aRect)
    {
        myCommandList->RSSetScissorRects(1, &aRect);
    }

    void GraphicsContext::SetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY aTopology)
    {
        myCommandList->IASetPrimitiveTopology(aTopology);
    }

    void GraphicsContext::SetStencilRef(const uint32_t aRef)
    {
        myCommandList->OMSetStencilRef(aRef);
    }
    
    void GraphicsContext::SetRenderTargets(const std::span<const TextureResource* const> aRenderTargets, const TextureResource* aDepthStencil)
    {
        N_CORE_ASSERT(aRenderTargets.size() <= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "Too many render targets");

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{};

        for (size_t i{ 0 }; i < aRenderTargets.size(); ++i)
        {
            N_CORE_ASSERT(aRenderTargets[i] != nullptr, "Null render target");
            N_CORE_ASSERT(aRenderTargets[i]->RTVDescriptor.IsValid(), "Texture has no RTV");
            rtvHandles[i] = aRenderTargets[i]->RTVDescriptor.CPUHandle;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{};
        D3D12_CPU_DESCRIPTOR_HANDLE* dsvPtr{ nullptr };

        if (aDepthStencil != nullptr)
        {
            N_CORE_ASSERT(aDepthStencil->DSVDescriptor.IsValid(), "Texture has no DSV");
            dsvHandle = aDepthStencil->DSVDescriptor.CPUHandle;
            dsvPtr = &dsvHandle;
        }

        myCommandList->OMSetRenderTargets(static_cast<UINT>(aRenderTargets.size()), rtvHandles, false, dsvPtr);
    }
    
    void GraphicsContext::ClearRenderTarget(const TextureResource& aTarget, const float aColor[4])
    {
        N_CORE_ASSERT(aTarget.RTVDescriptor.IsValid(), "Texture has no RTV");
        myCommandList->ClearRenderTargetView(aTarget.RTVDescriptor.CPUHandle, aColor, 0, nullptr);
    }

    void GraphicsContext::ClearDepthStencil(const TextureResource& aTarget, const float aDepth, const uint8_t aStencil)
    {
        N_CORE_ASSERT(aTarget.DSVDescriptor.IsValid(), "Texture has no DSV");
        myCommandList->ClearDepthStencilView(aTarget.DSVDescriptor.CPUHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, aDepth, aStencil, 0, nullptr);
    }
    
    void GraphicsContext::Draw(const uint32_t aVertexCount, const uint32_t aStartVertex)
    {
        FlushBarriers();
        myCommandList->DrawInstanced(aVertexCount, 1, aStartVertex, 0);
    }

    void GraphicsContext::DrawIndexed(const uint32_t aIndexCount, const uint32_t aStartIndex, const uint32_t aBaseVertex)
    {
        FlushBarriers();
        myCommandList->DrawIndexedInstanced(aIndexCount, 1, aStartIndex, aBaseVertex, 0);
    }

    void GraphicsContext::DrawInstanced(const uint32_t aVertexCountPerInstance, const uint32_t aInstanceCount, const uint32_t aStartVertex, const uint32_t aStartInstance)
    {
        FlushBarriers();
        myCommandList->DrawInstanced(aVertexCountPerInstance, aInstanceCount, aStartVertex, aStartInstance);
    }
    
    void GraphicsContext::DrawIndexedInstanced(const uint32_t aIndexCountPerInstance, const uint32_t aInstanceCount, const uint32_t aStartIndex, const uint32_t aBaseVertex, const uint32_t aStartInstance)
    {
        FlushBarriers();
        myCommandList->DrawIndexedInstanced(aIndexCountPerInstance, aInstanceCount, aStartIndex, aBaseVertex, aStartInstance);
    }

    void GraphicsContext::DrawFullScreenTriangle()
    {
        SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        myCommandList->IASetIndexBuffer(nullptr);
        Draw(3);
    }
    
    void GraphicsContext::Dispatch(const uint32_t aGroupX, const uint32_t aGroupY, const uint32_t aGroupZ)
    {
        FlushBarriers();
        myCommandList->Dispatch(aGroupX, aGroupY, aGroupZ);
    }

    void GraphicsContext::Dispatch1D(const uint32_t aThreadCountX, const uint32_t aGroupSizeX)
    {
        Dispatch((aThreadCountX + aGroupSizeX - 1) / aGroupSizeX, 1, 1);
    }

    void GraphicsContext::Dispatch2D(const uint32_t aThreadCountX, const uint32_t aThreadCountY, const uint32_t aGroupSizeX, const uint32_t aGroupSizeY)
    {
        Dispatch((aThreadCountX + aGroupSizeX - 1) / aGroupSizeX, (aThreadCountY + aGroupSizeY - 1) / aGroupSizeY, 1);
    }

    void GraphicsContext::Dispatch3D(const uint32_t aThreadCountX, const uint32_t aThreadCountY, const uint32_t aThreadCountZ, const uint32_t aGroupSizeX, const uint32_t aGroupSizeY, const uint32_t aGroupSizeZ)
    {
        Dispatch((aThreadCountX + aGroupSizeX - 1) / aGroupSizeX, (aThreadCountY + aGroupSizeY - 1) / aGroupSizeY, (aThreadCountZ + aGroupSizeZ - 1) / aGroupSizeZ);
    }
}