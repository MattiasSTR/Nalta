#pragma once
#include "RenderFrame.h"
#include "RenderView.h"

namespace Nalta::Graphics
{
    class GPUResourceManager;

    class SceneTranslator
    {
    public:
        explicit SceneTranslator(GPUResourceManager* aGpuResourceSystem);

        void Translate(const SceneView& aSceneView, const CameraView& aCamera, RenderView& aOutRenderView);

    private:
        GPUResourceManager* myGpuResourceSystem{ nullptr };
    };
}
