#include "npch.h"
#include "Nalta/RHI/D3D12/Contexts/D3D12Context.h"
#include "Nalta/RHI/D3D12/D3D12Device.h"
#include "Nalta/RHI/D3D12/Common/D3D12Descriptor.h"

namespace Nalta::RHI::D3D12
{
    Context::Context(Device& aDevice, const D3D12_COMMAND_LIST_TYPE aType)
        : myDevice(aDevice)
        , myType(aType)
    {
        for (uint32_t i{ 0 }; i < FRAMES_IN_FLIGHT; ++i)
        {
            NL_DX_VERIFY(aDevice.GetD3D12Device()->CreateCommandAllocator(aType, IID_PPV_ARGS(&myCommandAllocators[i])));
        }

        NL_DX_VERIFY(aDevice.GetD3D12Device()->CreateCommandList1(0, aType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&myCommandList)));
    }
    
    Context::~Context()
    {
        SafeRelease(myCommandList);

        for (auto& allocator : myCommandAllocators)
        {
            SafeRelease(allocator);
        }
    }
    
    void Context::Reset()
    {
        const uint32_t frameIndex{ myDevice.GetFrameIndex() };

        // Close the command list if it's still open from last frame
        if (myIsRecording)
        {
            NL_DX_VERIFY(myCommandList->Close());
            myIsRecording = false;
        }

        NL_DX_VERIFY(myCommandAllocators[frameIndex]->Reset());
        NL_DX_VERIFY(myCommandList->Reset(myCommandAllocators[frameIndex], nullptr));
        myIsRecording = true;

        if (myType != D3D12_COMMAND_LIST_TYPE_COPY)
        {
            BindDescriptorHeaps();
        }
    }
    
    void Context::BindDescriptorHeaps()
    {
        // For fully bindless we bind once - shaders index directly into these
        ID3D12DescriptorHeap* heaps[]
        {
            myDevice.GetBindlessHeap().GetHeap(),
            myDevice.GetSamplerHeap().GetHeap()
        };

        myCommandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);
    }
    
    void Context::AddBarrier(Resource& aResource, const D3D12_RESOURCE_STATES aNewState)
    {
        if (myNumQueuedBarriers >= MAX_QUEUED_BARRIERS)
        {
            FlushBarriers();
        }

        const D3D12_RESOURCE_STATES oldState{ aResource.state };

        if (myType == D3D12_COMMAND_LIST_TYPE_COMPUTE)
        {
            constexpr D3D12_RESOURCE_STATES ValidComputeStates{
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS        |
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                D3D12_RESOURCE_STATE_COPY_DEST               |
                D3D12_RESOURCE_STATE_COPY_SOURCE
            };
            N_CORE_ASSERT((oldState & ValidComputeStates) == oldState, "Invalid resource state for compute context");
            N_CORE_ASSERT((aNewState & ValidComputeStates) == aNewState, "Invalid target state for compute context");
        }

        if (oldState == aNewState)
        {
            // UAV barrier - needed to sync UAV writes with subsequent reads
            if (aNewState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            {
                D3D12_RESOURCE_BARRIER& barrier{ myBarriers[myNumQueuedBarriers++] };
                barrier.Type      = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                barrier.Flags     = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrier.UAV.pResource = aResource.resource;
            }
            return;
        }

        D3D12_RESOURCE_BARRIER& barrier{ myBarriers[myNumQueuedBarriers++] };
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = aResource.resource;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = oldState;
        barrier.Transition.StateAfter  = aNewState;

        aResource.state = aNewState;
    }
    
    void Context::FlushBarriers()
    {
        if (myNumQueuedBarriers == 0)
        {
            return;
        }

        myCommandList->ResourceBarrier(myNumQueuedBarriers, myBarriers.data());
        myNumQueuedBarriers = 0;
    }

    void Context::Close()
    {
        N_CORE_ASSERT(myIsRecording, "Closing a command list that is not recording");
        NL_DX_VERIFY(myCommandList->Close());
        myIsRecording = false;
    }

    void Context::CopyResource(Resource& aDst, const Resource& aSrc)
    {
        myCommandList->CopyResource(aDst.resource, aSrc.resource);
    }

    void Context::CopyBufferRegion(Resource& aDst, const uint64_t aDstOffset, const Resource& aSrc, const uint64_t aSrcOffset, const uint64_t aNumBytes)
    {
        myCommandList->CopyBufferRegion(aDst.resource, aDstOffset, aSrc.resource, aSrcOffset, aNumBytes);
    }
}
