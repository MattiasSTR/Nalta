#pragma once
#include "Nalta/Core/Math.h"
#include "Nalta/Core/SceneView.h"
#include "Nalta/Graphics/GPUResourceKeys.h"

#include <vector>

namespace Nalta::Graphics
{
    struct SurfaceView
    {
        RenderSurfaceKey surface;
        const SceneView* scene{ nullptr };
        uint32_t width{ 0 };
        uint32_t height{ 0 };
    };


    struct RenderFrame
    {
        SceneView scene;
        std::vector<SurfaceView> surfaces;

        void Reset() { scene.Reset(); surfaces.clear(); }
    };
}