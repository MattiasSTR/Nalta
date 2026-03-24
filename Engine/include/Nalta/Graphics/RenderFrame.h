#pragma once
#include "Nalta/Graphics/RenderCommands.h"
#include <vector>

namespace Nalta
{
    struct RenderFrame
    {
        std::vector<Graphics::RenderCommand> commands;

        void SetPipeline(const Graphics::PipelineHandle aPipeline)
        {
            commands.emplace_back(Graphics::SetPipelineCmd{ aPipeline });
        }
        
        void SetVertexBuffer(const Graphics::VertexBufferHandle aBuffer)
        {
            commands.emplace_back(Graphics::SetVertexBufferCmd{ aBuffer });
        }
        
        void SetIndexBuffer(const Graphics::IndexBufferHandle aBuffer)
        {
            commands.emplace_back(Graphics::SetIndexBufferCmd{ aBuffer });
        }
        
        void UpdateConstantBuffer(const Graphics::ConstantBufferHandle aBuffer, const void* aData, const uint32_t aSize)
        {
            const auto* bytes{ static_cast<const std::byte*>(aData) };
            commands.emplace_back(Graphics::UpdateConstantBufferCmd
            {
                .buffer = aBuffer,
                .data = std::vector<std::byte>(bytes, bytes + aSize)
            });
        }

        void SetConstantBuffer(const Graphics::ConstantBufferHandle aBuffer, const uint32_t aRootParameterIndex = 0)
        {
            commands.emplace_back(Graphics::SetConstantBufferCmd
            {
                .buffer = aBuffer,
                .rootParameterIndex = aRootParameterIndex
            });
        }

        void Draw(const uint32_t aVertexCount, const uint32_t aInstanceCount = 1, const uint32_t aVertexOffset = 0, const uint32_t aInstanceOffset = 0)
        {
            commands.emplace_back(Graphics::DrawCmd
            {
                .vertexCount    = aVertexCount,
                .instanceCount  = aInstanceCount,
                .vertexOffset   = aVertexOffset,
                .instanceOffset = aInstanceOffset
            });
        }

        void DrawIndexed(const uint32_t aIndexCount, const uint32_t aInstanceCount = 1, const uint32_t aIndexOffset = 0, 
            const int32_t aVertexOffset = 0, const uint32_t aInstanceOffset = 0)
        {
            commands.emplace_back(Graphics::DrawIndexedCmd
            {
                .indexCount     = aIndexCount,
                .instanceCount  = aInstanceCount,
                .indexOffset    = aIndexOffset,
                .vertexOffset   = aVertexOffset,
                .instanceOffset = aInstanceOffset
            });
        }

        void Clear() { commands.clear(); }
    };
}