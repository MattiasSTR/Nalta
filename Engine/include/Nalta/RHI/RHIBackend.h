#pragma once
#include "Nalta/Core/PlatformDetector.h"

#define N_RHI_D3D12

#if defined(N_RHI_D3D12) && !defined(N_PLATFORM_WINDOWS)
    #error "D3D12 only works on Windows"
#endif

#if !defined(N_RHI_D3D12) && !defined(N_RHI_VULKAN)
    #error "No RHI backend defined. Set N_RHI_D3D12 or N_RHI_VULKAN in your build configuration."
#endif

#if defined(N_RHI_D3D12) && defined(N_RHI_VULKAN)
    #error "Cannot define multiple RHI backends simultaneously."
#endif