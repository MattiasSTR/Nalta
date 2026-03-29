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
    };
    
    inline const char* AssetTypeToString(const AssetType aType)
    {
        switch (aType)
        {
            case AssetType::Mesh:     return "Mesh";
            case AssetType::Texture:  return "Texture";
            case AssetType::Material: return "Material";
            default:                  return "Unknown";
        }
    }
}