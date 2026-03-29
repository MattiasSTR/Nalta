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
    
    inline CullMode CullModeFromString(const std::string& aStr)
    {
        if (aStr == "None")  return CullMode::None;
        if (aStr == "Front") return CullMode::Front;
        if (aStr == "Back")  return CullMode::Back;
        NL_WARN(GCoreLogger, "PipelineDesc: unknown CullMode '{}', defaulting to Back", aStr);
        return CullMode::Back;
    }
    
    inline std::string CullModeToString(const CullMode aMode)
    {
        switch (aMode)
        {
            case CullMode::None:  return "None";
            case CullMode::Front: return "Front";
            case CullMode::Back:  return "Back";
            default:              return "Back";
        }
    }
    
    inline FillMode FillModeFromString(const std::string& aStr)
    {
        if (aStr == "Solid")     return FillMode::Solid;
        if (aStr == "Wireframe") return FillMode::Wireframe;
        NL_WARN(GCoreLogger, "PipelineDesc: unknown FillMode '{}', defaulting to Solid", aStr);
        return FillMode::Solid;
    }
    
    inline std::string FillModeToString(const FillMode aMode)
    {
        switch (aMode)
        {
            case FillMode::Solid:     return "Solid";
            case FillMode::Wireframe: return "Wireframe";
            default:                  return "Solid";
        }
    }
    
    inline DepthCompare DepthCompareFromString(const std::string& aStr)
    {
        if (aStr == "Less")         return DepthCompare::Less;
        if (aStr == "LessEqual")    return DepthCompare::LessEqual;
        if (aStr == "Greater")      return DepthCompare::Greater;
        if (aStr == "GreaterEqual") return DepthCompare::GreaterEqual;
        if (aStr == "Equal")        return DepthCompare::Equal;
        if (aStr == "Always")       return DepthCompare::Always;
        NL_WARN(GCoreLogger, "PipelineDesc: unknown DepthCompare '{}', defaulting to Greater", aStr);
        return DepthCompare::Greater;
    }
    
    inline std::string DepthCompareToString(const DepthCompare aCompare)
    {
        switch (aCompare)
        {
            case DepthCompare::Less:         return "Less";
            case DepthCompare::LessEqual:    return "LessEqual";
            case DepthCompare::Greater:      return "Greater";
            case DepthCompare::GreaterEqual: return "GreaterEqual";
            case DepthCompare::Equal:        return "Equal";
            case DepthCompare::Always:       return "Always";
            default:                         return "Greater";
        }
    }
}