#include "npch.h"
#include "Nalta/RHI/D3D12/Common/D3D12Queue.h"
#include "Nalta/RHI/D3D12/D3D12Device.h"

namespace Nalta::RHI::D3D12
{
    namespace
    {
        constexpr D3D12_COMMAND_LIST_TYPE ToD3D12CommandListType(const QueueType aType)
        {
            switch (aType)
            {
                case QueueType::Graphics: return D3D12_COMMAND_LIST_TYPE_DIRECT;
                case QueueType::Compute: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
                case QueueType::Copy: return D3D12_COMMAND_LIST_TYPE_COPY;
                default:
                {
                    N_CORE_ASSERT(false, "Unknown QueueType");
                    return D3D12_COMMAND_LIST_TYPE_DIRECT;
                }
            }
        }
        
        constexpr const char* QueueTypeToString(const QueueType aType)
        {
            switch (aType)
            {
                case QueueType::Graphics: return "Graphics";
                case QueueType::Compute:  return "Compute";
                case QueueType::Copy:     return "Copy";
                default:                  return "Unknown";
            }
        }
    }
    
    Queue::Queue(Device& aDevice, QueueType aType)
        : myDevice(aDevice)
        , myType(aType)
    {
        const D3D12_COMMAND_LIST_TYPE commandListType{ ToD3D12CommandListType(aType) };

        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = commandListType;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;

        NL_DX_VERIFY(aDevice.GetD3D12Device()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&myCommandQueue)));
        NL_DX_VERIFY(aDevice.GetD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&myFence)));

        myFence->Signal(myLastCompletedFenceValue);

        myFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        N_CORE_ASSERT(myFenceEvent != nullptr, "Failed to create fence event");

        // Debug naming
        switch (aType)
        {
            case QueueType::Graphics:
                N_D3D12_SET_NAME(myCommandQueue, "Graphics Queue");
                N_D3D12_SET_NAME(myFence, "Graphics Queue Fence");
                break;
            case QueueType::Compute:
                N_D3D12_SET_NAME(myCommandQueue, "Compute Queue");
                N_D3D12_SET_NAME(myFence, "Compute Queue Fence");
                break;
            case QueueType::Copy:
                N_D3D12_SET_NAME(myCommandQueue, "Copy Queue");
                N_D3D12_SET_NAME(myFence, "Copy Queue Fence");
                break;
            default:
                break;
        }

        NL_TRACE(GCoreLogger, "{} queue initialized", QueueTypeToString(aType));
    }
    
    Queue::~Queue()
    {
        WaitForIdle();

        CloseHandle(myFenceEvent);
        SafeRelease(myFence);
        SafeRelease(myCommandQueue);
    }
    
    void Queue::InsertWait(const uint64_t aFenceValue)
    {
        myCommandQueue->Wait(myFence, aFenceValue);
    }

    void Queue::InsertWaitForQueue(const Queue& aOtherQueue)
    {
        myCommandQueue->Wait(aOtherQueue.GetFence(), aOtherQueue.GetNextFenceValue() - 1);
    }

    void Queue::InsertWaitForQueueFence(const Queue& aOtherQueue, const uint64_t aFenceValue)
    {
        myCommandQueue->Wait(aOtherQueue.GetFence(), aFenceValue);
    }
    
    void Queue::WaitForFenceCPUBlocking(const uint64_t aFenceValue)
    {
        if (IsFenceComplete(aFenceValue))
        {
            return;
        }

        std::lock_guard<std::mutex> lock(myEventMutex);
        NL_DX_VERIFY(myFence->SetEventOnCompletion(aFenceValue, myFenceEvent));
        WaitForSingleObjectEx(myFenceEvent, INFINITE, false);
        myLastCompletedFenceValue = aFenceValue;
    }
    
    void Queue::WaitForIdle()
    {
        WaitForFenceCPUBlocking(myNextFenceValue - 1);
    }
    
    uint64_t Queue::ExecuteCommandLists(const std::span<ID3D12CommandList* const> aCommandLists)
    {
        // Close is the caller's responsibility before passing here.
        // We assert in debug that the lists are non-null but trust the caller closed them.
        N_CORE_ASSERT(!aCommandLists.empty(), "Submitted empty command list span");

        myCommandQueue->ExecuteCommandLists(
            static_cast<UINT>(aCommandLists.size()),
            aCommandLists.data()
        );

        return SignalFence();
    }
    
    uint64_t Queue::SignalFence()
    {
        std::lock_guard<std::mutex> lock(myFenceMutex);
        NL_DX_VERIFY(myCommandQueue->Signal(myFence, myNextFenceValue));
        return myNextFenceValue++;
    }
    
    bool Queue::IsFenceComplete(const uint64_t aFenceValue)
    {
        if (aFenceValue > myLastCompletedFenceValue)
        {
            PollCurrentFenceValue();
        }

        return aFenceValue <= myLastCompletedFenceValue;
    }
    
    uint64_t Queue::PollCurrentFenceValue()
    {
        myLastCompletedFenceValue = std::max(myLastCompletedFenceValue, myFence->GetCompletedValue());
        return myLastCompletedFenceValue;
    }
}