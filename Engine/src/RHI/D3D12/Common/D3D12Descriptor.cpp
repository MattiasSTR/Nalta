#include "npch.h"
#include "Nalta/RHI/D3D12/Common/D3D12Descriptor.h"

namespace Nalta::RHI::D3D12
{
    DescriptorHeap::DescriptorHeap(ID3D12Device* aDevice, const D3D12_DESCRIPTOR_HEAP_TYPE aType, const uint32_t aCapacity, const bool aShaderVisible)
        : myType(aType)
        , myCapacity(aCapacity)
        , myShaderVisible(aShaderVisible)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = myType;
        desc.NumDescriptors = myCapacity;
        desc.Flags = myShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 0;

        NL_DX_VERIFY(aDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&myHeap)));

        myDescriptorSize = aDevice->GetDescriptorHandleIncrementSize(myType);

        myHeapStart.CPUHandle = myHeap->GetCPUDescriptorHandleForHeapStart();
        myHeapStart.heapIndex = 0;

        if (myShaderVisible)
        {
            myHeapStart.GPUHandle = myHeap->GetGPUDescriptorHandleForHeapStart();
        }
    }
    
    DescriptorHeap::~DescriptorHeap()
    {
        SafeRelease(myHeap);
    }
    
    Descriptor DescriptorHeap::ComputeDescriptor(const uint32_t aIndex) const
    {
        N_CORE_ASSERT(aIndex < myCapacity, "Descriptor index out of range");

        Descriptor descriptor;
        descriptor.heapIndex = aIndex;
        descriptor.CPUHandle.ptr = myHeapStart.CPUHandle.ptr + static_cast<uint64_t>(aIndex) * myDescriptorSize;

        if (myShaderVisible)
        {
            descriptor.GPUHandle.ptr = myHeapStart.GPUHandle.ptr + static_cast<uint64_t>(aIndex) * myDescriptorSize;
        }

        return descriptor;
    }
    
    FreeListDescriptorHeap::FreeListDescriptorHeap(ID3D12Device* aDevice, const D3D12_DESCRIPTOR_HEAP_TYPE aType, const uint32_t aCapacity, const bool aShaderVisible)
       : DescriptorHeap(aDevice, aType, aCapacity, aShaderVisible)
    {
        myFreeList.reserve(aCapacity);
    }
    
    FreeListDescriptorHeap::~FreeListDescriptorHeap()
    {
        N_CORE_ASSERT(myAllocatedCount == 0, "Descriptor heap destroyed with live descriptors - possible leak");
    }
    
    Descriptor FreeListDescriptorHeap::Allocate()
    {
        std::lock_guard<std::mutex> lock(myMutex);

        uint32_t index{ 0 };

        if (!myFreeList.empty())
        {
            index = myFreeList.back();
            myFreeList.pop_back();
        }
        else
        {
            N_CORE_ASSERT(myCurrentIndex < myCapacity, "Descriptor heap is full - increase capacity");
            index = myCurrentIndex++;
        }

        myAllocatedCount++;
        return ComputeDescriptor(index);
    }
    
    void FreeListDescriptorHeap::Free(const Descriptor& aDescriptor)
    {
        N_CORE_ASSERT(aDescriptor.IsValid(), "Freeing invalid descriptor");
        N_CORE_ASSERT(aDescriptor.heapIndex < myCapacity, "Descriptor index out of range");

        std::lock_guard<std::mutex> lock(myMutex);

        myFreeList.push_back(aDescriptor.heapIndex);

        N_CORE_ASSERT(myAllocatedCount > 0, "Freeing descriptor when none are allocated");
        myAllocatedCount--;
    }
}