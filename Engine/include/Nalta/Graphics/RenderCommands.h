#pragma once
#include "IndexBufferHandle.h"
#include "PipelineHandle.h"
#include "VertexBufferHandle.h"
#include "ConstantBufferHandle.h"

#include <cstdint>
#include <variant>
#include <vector>

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

    struct UpdateConstantBufferCmd
    {
        ConstantBufferHandle        buffer;
        std::vector<std::byte>      data;
    };

    struct SetConstantBufferCmd
    {
        ConstantBufferHandle buffer;
        uint32_t             rootParameterIndex{ 0 };
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
        UpdateConstantBufferCmd,
        SetConstantBufferCmd,
        DrawCmd,
        DrawIndexedCmd
    >;
}
