#pragma once
#include "IndexBufferHandle.h"
#include "PipelineHandle.h"
#include "VertexBufferHandle.h"

#include <cstdint>
#include <variant>

namespace Nalta::Graphics
{
    struct SetPipelineCmd
    {
        PipelineHandle pipeline;
    };

    struct SetVertexBufferCmd
    {
        VertexBufferHandle buffer;
    };
    
    struct SetIndexBufferCmd
    {
        IndexBufferHandle buffer;
    };


    struct DrawCmd
    {
        uint32_t vertexCount   { 0 };
        uint32_t instanceCount { 1 };
        uint32_t vertexOffset  { 0 };
        uint32_t instanceOffset{ 0 };
    };

    struct DrawIndexedCmd
    {
        uint32_t indexCount    { 0 };
        uint32_t instanceCount { 1 };
        uint32_t indexOffset   { 0 };
        int32_t  vertexOffset  { 0 };
        uint32_t instanceOffset{ 0 };
    };

    using RenderCommand = std::variant<
        SetPipelineCmd,
        SetVertexBufferCmd,
        SetIndexBufferCmd,
        DrawCmd,
        DrawIndexedCmd
    >;
}
