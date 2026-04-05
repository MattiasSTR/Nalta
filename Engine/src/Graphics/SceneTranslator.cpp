#include "npch.h"
#include "Nalta/Graphics/SceneTranslator.h"
#include "Nalta/Graphics/RenderView.h"
#include "Nalta/Core/SceneView.h"

namespace Nalta::Graphics
{
    SceneTranslator::SceneTranslator(GPUResourceManager* aGpuResourceSystem)
        : myGpuResourceSystem(aGpuResourceSystem)
    {}

    void SceneTranslator::Translate(const SceneView&, const CameraView&, RenderView&)
    {
        // Straight passthrough for now
    }
}