#pragma once
#include "Nalta/Core/Math.h"
#include "Nalta/Core/SceneView.h"
#include "Nalta/Graphics/GPUResourceKeys.h"

#include <vector>

namespace Nalta::Graphics
{
    struct CameraView
    {
        float4x4 view;
        float4x4 projection;
        float3 position;

        float viewportX{ 0.0f };
        float viewportY{ 0.0f };
        float viewportWidth{ 1.0f };
        float viewportHeight{ 1.0f };
    };

    struct SurfaceView
    {
        RenderSurfaceKey surface;
        std::vector<CameraView> cameras;
        uint32_t width{ 0 };
        uint32_t height{ 0 };
    };

    struct RenderFrame
    {
        SceneView scene;
        std::vector<SurfaceView> surfaces;

        void Reset()
        {
            scene.Reset();
            surfaces.clear();
        }
    };
}