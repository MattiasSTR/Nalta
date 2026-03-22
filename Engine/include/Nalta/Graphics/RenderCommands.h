#pragma once
#include "Nalta/Graphics/PipelineHandle.h"

#include <cstdint>
#include <variant>

namespace Nalta::Graphics
{
    struct SetPipelineCmd
    {
        PipelineHandle pipeline;
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
        DrawCmd,
        DrawIndexedCmd
    >;
}
