#pragma once
#include "Nalta/RHI/RHIBackend.h"

#if defined(N_RHI_D3D12)
    #include "Nalta/RHI/D3D12/D3D12Device.h"
    namespace Nalta::RHI { using Device = D3D12::Device; }
#elif defined(N_RHI_VULKAN)
    #include "Nalta/RHI/Vulkan/VulkanDevice.h"
    namespace Nalta::RHI { using Device = Vulkan::Device; }
#endif