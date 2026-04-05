#pragma once
#include "Nalta/RHI/D3D12/Common/D3D12Common.h"

#include <mutex>

namespace Nalta::RHI::D3D12
{
    struct Descriptor
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle{ 0 };
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle{ 0 };
        uint32_t heapIndex{ UINT32_MAX };

        bool IsValid() const { return CPUHandle.ptr != 0; }
        bool IsShaderVisible() const { return GPUHandle.ptr != 0; }
    };

    // Base - owns the heap and descriptor size, nothing else
    class DescriptorHeap
    {
    public:
        DescriptorHeap(ID3D12Device* aDevice, D3D12_DESCRIPTOR_HEAP_TYPE aType, uint32_t aCapacity, bool aShaderVisible);
        virtual ~DescriptorHeap();

        DescriptorHeap(const DescriptorHeap&) = delete;
        DescriptorHeap& operator=(const DescriptorHeap&) = delete;
        DescriptorHeap(DescriptorHeap&&) = delete;
        DescriptorHeap& operator=(DescriptorHeap&&) = delete;

        [[nodiscard]] ID3D12DescriptorHeap* GetHeap() const { return myHeap; }
        [[nodiscard]] uint32_t GetCapacity() const { return myCapacity; }
        [[nodiscard]] uint32_t GetDescriptorSize() const { return myDescriptorSize; }
        [[nodiscard]] D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return myType; }

    protected:
        [[nodiscard]] Descriptor ComputeDescriptor(uint32_t aIndex) const;

        ID3D12DescriptorHeap* myHeap{ nullptr };
        D3D12_DESCRIPTOR_HEAP_TYPE myType;
        uint32_t myCapacity{ 0 };
        uint32_t myDescriptorSize{ 0 };
        bool myShaderVisible{ false };
        Descriptor myHeapStart{};
    };

    // Free list heap - for bindless SRV/UAV/CBV, RTV staging, DSV staging
    class FreeListDescriptorHeap final : public DescriptorHeap
    {
    public:
        FreeListDescriptorHeap(ID3D12Device* aDevice, D3D12_DESCRIPTOR_HEAP_TYPE aType, uint32_t aCapacity, bool aShaderVisible);
        ~FreeListDescriptorHeap() override;

        Descriptor Allocate();
        void Free(const Descriptor& aDescriptor);

        [[nodiscard]] uint32_t GetAllocatedCount() const { return myAllocatedCount; }

    private:
        std::vector<uint32_t> myFreeList;
        uint32_t myCurrentIndex{ 0 };
        uint32_t myAllocatedCount{ 0 };
        std::mutex myMutex;
    };
}
