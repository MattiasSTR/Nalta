#pragma once
#include "Nalta/RHI/RHIBackend.h"

#ifdef N_RHI_D3D12
#include "Nalta/RHI/D3D12/D3D12RenderSurface.h"
#include "Nalta/RHI/D3D12/Resources/D3D12Buffer.h"
#include "Nalta/RHI/D3D12/Resources/D3D12PipelineStateObject.h"
#include "Nalta/RHI/D3D12/Resources/D3D12Texture.h"

namespace Nalta::RHI
{
    using Buffer        = D3D12::BufferResource;
    using Texture       = D3D12::TextureResource;
    using Pipeline      = D3D12::PipelineStateObject;
    using RenderSurface = D3D12::RenderSurface;
}

#endif