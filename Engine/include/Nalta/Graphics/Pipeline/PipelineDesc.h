#pragma once
#include "Nalta/Graphics/Shader/Shader.h"

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
    
    enum class DepthCompare : uint8_t
    {
        Less,
        LessEqual,
        Greater,        // reversed-Z
        GreaterEqual,   // reversed-Z
        Equal,
        Always
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
        DepthCompare compareFunc { DepthCompare::Greater }; // reversed-Z default
    };

    struct PipelineDesc
    {
        std::shared_ptr<Shader> shader;
        RasterizerDesc rasterizer;
        BlendDesc blend;
        DepthDesc depth;
    };
}