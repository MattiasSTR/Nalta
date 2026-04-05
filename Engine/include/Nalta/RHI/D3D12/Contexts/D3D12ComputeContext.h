#pragma once
#include "D3D12Context.h"

namespace Nalta::RHI::D3D12
{
    struct PipelineStateObject;

    class ComputeContext final : public Context
    {
    public:
        explicit ComputeContext(Device& aDevice);

        void Reset() override;

        // Pipeline
        void SetPipeline(const PipelineStateObject& aPipeline);

        // Binding
        void SetRootConstants(const void* aData, uint32_t aCount, uint32_t aOffset = 0);
        void SetPassCBV(uint64_t aGPUAddress);

        template<typename T>
        void SetRootConstants(const T& aData, uint32_t aOffset = 0)
        {
            static_assert(sizeof(T) % 4 == 0, "Root constant struct must be 4-byte aligned");
            static_assert(sizeof(T) / 4 <= ROOT_CONSTANT_COUNT, "Root constant struct too large");
            SetRootConstants(&aData, sizeof(T) / 4, aOffset);
        }

        // Dispatch
        void Dispatch(uint32_t aGroupX, uint32_t aGroupY, uint32_t aGroupZ);
        void Dispatch1D(uint32_t aThreadCountX, uint32_t aGroupSizeX);
        void Dispatch2D(uint32_t aThreadCountX, uint32_t aThreadCountY, uint32_t aGroupSizeX, uint32_t aGroupSizeY);
        void Dispatch3D(uint32_t aThreadCountX, uint32_t aThreadCountY, uint32_t aThreadCountZ, uint32_t aGroupSizeX, uint32_t aGroupSizeY, uint32_t aGroupSizeZ);
    };
}