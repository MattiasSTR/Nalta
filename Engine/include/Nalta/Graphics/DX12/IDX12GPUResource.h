#pragma once
#include "Nalta/Graphics/Buffers/IGPUResource.h"

struct ID3D12Resource;

namespace Nalta::Graphics
{
    class IDX12GPUResource : public virtual IGPUResource
    {
    public:
        virtual void SetResource(ID3D12Resource* aResource) = 0;
    };
}