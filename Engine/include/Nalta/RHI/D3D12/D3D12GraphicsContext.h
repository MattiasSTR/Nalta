#pragma once
#include "D3D12Context.h"

namespace Nalta::RHI::D3D12
{
    class GraphicsContext final : public Context
    {
    public:
        explicit GraphicsContext(Device& aDevice);
    
        // Viewport and scissor
        void SetViewport(uint32_t aWidth, uint32_t aHeight);
        void SetViewport(const D3D12_VIEWPORT& aViewport);
        void SetScissor(uint32_t aWidth, uint32_t aHeight);
        void SetScissor(const D3D12_RECT& aRect);
    
        // State
        void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY aTopology);
        void SetStencilRef(uint32_t aRef);
    
        // Render targets
        void SetRenderTargets(std::span<const TextureResource* const> aRenderTargets, const TextureResource* aDepthStencil = nullptr);
        void ClearRenderTarget(const TextureResource& aTarget, const float aColor[4]);
        void ClearDepthStencil(const TextureResource& aTarget, float aDepth = 0.0f, uint8_t aStencil = 0);
    
        // Draw
        void Draw(uint32_t aVertexCount, uint32_t aStartVertex = 0);
        void DrawIndexed(uint32_t aIndexCount, uint32_t aStartIndex  = 0, uint32_t aBaseVertex  = 0);
        void DrawInstanced(uint32_t aVertexCountPerInstance, uint32_t aInstanceCount, uint32_t aStartVertex = 0, uint32_t aStartInstance = 0);
        void DrawIndexedInstanced(uint32_t aIndexCountPerInstance, uint32_t aInstanceCount, uint32_t aStartIndex, uint32_t aBaseVertex, uint32_t aStartInstance);
        void DrawFullScreenTriangle();
    
        // Compute on graphics queue
        void Dispatch(uint32_t aGroupX, uint32_t aGroupY, uint32_t aGroupZ);
        void Dispatch1D(uint32_t aThreadCountX, uint32_t aGroupSizeX);
        void Dispatch2D(uint32_t aThreadCountX, uint32_t aThreadCountY, uint32_t aGroupSizeX, uint32_t aGroupSizeY);
        void Dispatch3D(uint32_t aThreadCountX, uint32_t aThreadCountY, uint32_t aThreadCountZ, uint32_t aGroupSizeX, uint32_t aGroupSizeY, uint32_t aGroupSizeZ);
    };
}