#pragma once
#include "D3D12Resource.h"
#include "Nalta/RHI/RHITypes.h"
#include "Nalta/RHI/D3D12/D3D12Common.h"
#include "Nalta/RHI/D3D12/D3D12Descriptor.h"

#include <D3D12MemAlloc.h>

namespace Nalta::RHI::D3D12
{
    struct BufferResource : Resource
    {
        BufferResource()
        {
            type = ResourceType::Buffer;
        }

        uint8_t* mappedData{ nullptr }; // non-null only for CpuToGpu/GpuToCpu
        uint32_t stride{ 0 };
        uint64_t gpuAddress{ 0 }; // virtual address, used for CBV and ray tracing

        // Descriptors - only valid if corresponding view flag was set at creation
        Descriptor CBVDescriptor{};
        Descriptor SRVDescriptor{};
        Descriptor UAVDescriptor{};

        [[nodiscard]] uint32_t GetBindlessIndex() const { return SRVDescriptor.heapIndex; }
    };
}