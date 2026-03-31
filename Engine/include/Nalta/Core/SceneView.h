#pragma once
#include "Nalta/Core/Math.h"
#include "Nalta/Assets/AssetKeys.h"

#include <vector>

namespace Nalta
{
    struct MeshDrawEntry
    {
        MeshKey mesh;
        PipelineKey pipeline;
        TextureKey albedo;
        float4x4 transform;
    };

    struct SceneCamera
    {
        float4x4 view;
        float4x4 projection;
        float4x4 viewProjection;
        float3 position;
    };

    struct SceneView
    {
        SceneCamera camera;
        std::vector<MeshDrawEntry> meshes;

        void Reset()
        {
            meshes.clear();
        }
    };
}