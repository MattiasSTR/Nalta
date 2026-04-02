#pragma once
#include "Nalta/RHI/RHI.h"
#include "Nalta/RHI/D3D12/D3D12Common.h"

#include <mutex>
#include <span>

namespace Nalta::RHI::D3D12
{
    class Device;

    class Queue final
    {
    public:
        Queue(Device& aDevice, QueueType aType);
        ~Queue();

        Queue(const Queue&) = delete;
        Queue& operator=(const Queue&) = delete;
        Queue(Queue&&) = delete;
        Queue& operator=(Queue&&) = delete;

        // GPU-side waits - inserts a wait on this queue for another queue's fence value
        void InsertWait(uint64_t aFenceValue);
        void InsertWaitForQueue(const Queue& aOtherQueue);
        void InsertWaitForQueueFence(const Queue& aOtherQueue, uint64_t aFenceValue);

        // CPU-side waits
        void WaitForFenceCPUBlocking(uint64_t aFenceValue);
        void WaitForIdle();

        // Submission - returns fence value you can use to track completion
        uint64_t ExecuteCommandLists(std::span<ID3D12CommandList* const> aCommandLists);
        uint64_t SignalFence();

        // Fence queries
        bool IsFenceComplete(uint64_t aFenceValue);
        uint64_t PollCurrentFenceValue();
        uint64_t GetLastCompletedFenceValue() const { return myLastCompletedFenceValue; }
        uint64_t GetNextFenceValue() const { return myNextFenceValue; }

        ID3D12CommandQueue* GetCommandQueue() const { return myCommandQueue; }
        ID3D12Fence* GetFence() const { return myFence; }
        QueueType GetType() const { return myType; }

    private:
        Device& myDevice;
        QueueType myType;
        ID3D12CommandQueue* myCommandQueue{ nullptr };
        ID3D12Fence* myFence{ nullptr };
        HANDLE myFenceEvent{ nullptr };
        uint64_t myNextFenceValue{ 1 };
        uint64_t myLastCompletedFenceValue{ 0 };

        std::mutex myFenceMutex;
        std::mutex myEventMutex;
    };
}
