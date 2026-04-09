#pragma once
#include "Nalta/Core/Math.h"
#include "Nalta/Assets/AssetKeys.h"

#include <vector>

namespace Nalta
{
    struct StaticMeshDrawEntry
    {
        MeshKey mesh;
        TextureKey albedo;
        float4x4 transform;
    };

    struct CameraDesc
    {
        float4x4 view;
        float3 position;
        
        float fovY{ 75.0f };
        float nearPlane{ 0.1f };
        float farPlane{ 1000.0f };

        float viewportX{ 0.0f };
        float viewportY{ 0.0f };
        float viewportWidth{ 1.0f };
        float viewportHeight{ 1.0f };
    };

    struct SceneSnapshot
    {
        std::vector<CameraDesc> cameras;
        std::vector<StaticMeshDrawEntry> meshes;

        void Reset() { cameras.clear(); meshes.clear(); }
    };
}