#pragma once
#include "Nalta/RHI/RHITypes.h"
#include "Nalta/RHI/D3D12/D3D12Common.h"
#include "Nalta/RHI/D3D12/D3D12Descriptor.h"
#include <D3D12MemAlloc.h>

namespace Nalta::RHI::D3D12
{
    struct Resource
    {
        ID3D12Resource* resource{ nullptr };
        D3D12MA::Allocation* allocation{ nullptr };
        D3D12_RESOURCE_STATES state{ D3D12_RESOURCE_STATE_COMMON };
        ResourceType type{ ResourceType::Buffer };
        bool isReady{ false };

        bool IsValid() const { return resource != nullptr; }
    };
}

