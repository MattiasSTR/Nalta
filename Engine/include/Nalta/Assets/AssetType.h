#pragma once
#include <cstdint>

namespace Nalta
{
    enum class AssetType : uint8_t
    {
        Unknown,
        Mesh,
        Texture,
        Material,
        Pipeline,
    };
    
    inline const char* AssetTypeToString(const AssetType aType)
    {
        switch (aType)
        {
            case AssetType::Mesh:     return "Mesh";
            case AssetType::Texture:  return "Texture";
            case AssetType::Material: return "Material";
            case AssetType::Pipeline: return "Pipeline";
            default:                  return "Unknown";
        }
    }
}