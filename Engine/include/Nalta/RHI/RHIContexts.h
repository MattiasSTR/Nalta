#pragma once
#include "Nalta/RHI/RHIBackend.h"

#ifdef N_RHI_D3D12
#include "Nalta/RHI/D3D12/Contexts/D3D12ComputeContext.h"
#include "Nalta/RHI/D3D12/Contexts/D3D12Context.h"
#include "Nalta/RHI/D3D12/Contexts/D3D12GraphicsContext.h"
#include "Nalta/RHI/D3D12/Contexts/D3D12UploadContext.h"

namespace Nalta::RHI
{
    using GraphicsContext          = D3D12::GraphicsContext;
    using ComputeContext           = D3D12::ComputeContext;
    using UploadContext            = D3D12::UploadContext;
    using Context                  = D3D12::Context;
}
#endif