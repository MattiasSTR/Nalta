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