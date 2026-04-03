#pragma once
#include "Nalta/RHI/RHI.h"

namespace Nalta::RHI {

#if defined(N_RHI_D3D12)
    namespace D3D12 { class Device; }
    using Device = D3D12::Device;

#elif defined(N_RHI_VULKAN)
    namespace Vulkan { class Device; }
    using Device = Vulkan::Device;
#endif

}