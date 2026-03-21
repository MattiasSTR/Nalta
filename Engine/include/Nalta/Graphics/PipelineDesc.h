#pragma once
#include "Shader.h"
#include <memory>

namespace Nalta::Graphics
{
    enum class CullMode : uint8_t
    {
        None,
        Front,
        Back
    };

    enum class FillMode : uint8_t
    {
        Solid,
        Wireframe
    };

    struct RasterizerDesc
    {
        CullMode cullMode{ CullMode::Back };
        FillMode fillMode{ FillMode::Solid };
        bool frontCCW{ false };
    };

    struct BlendDesc
    {
        bool blendEnabled{ false };
    };

    struct DepthDesc
    {
        bool depthEnabled{ false };
        bool depthWrite{ false };
    };

    struct PipelineDesc
    {
        std::shared_ptr<Shader> shader;
        RasterizerDesc rasterizer;
        BlendDesc blend;
        DepthDesc depth;
    };
}