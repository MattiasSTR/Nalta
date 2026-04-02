#pragma once
#include "Nalta/Assets/AssetState.h"
//#include "Nalta/Graphics/Pipeline/PipelineHandle.h"

namespace Nalta
{
    struct Pipeline
    {
        //Graphics::PipelineHandle gpuHandle;
        AssetState state{ AssetState::Unloaded };
    };
}