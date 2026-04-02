#pragma once
#include "Nalta/Core/PlatformDetector.h"

// Define N_RHI_D3D12 or N_RHI_VULKAN via premake/compiler flags.
// This header just validates and documents the choice.

#define N_RHI_D3D12
//#define N_RHI_VULKAN

#if defined (N_RHI_D3D12) && !defined(N_PLATFORM_WINDOWS)
    #error "D3D12 only works on windows"
#endif

#if !defined(N_RHI_D3D12) && !defined(N_RHI_VULKAN)
    #error "No RHI backend defined. Set N_RHI_D3D12 or N_RHI_VULKAN in your build configuration."
#endif

#if defined(N_RHI_D3D12) && defined(N_RHI_VULKAN)
    #error "Cannot define multiple RHI backends simultaneously."
#endif

namespace Nalta::RHI
{
    enum class QueueType : uint8_t
    {
        Graphics = 0,
        Compute,
        Copy,
        Count
    };
}