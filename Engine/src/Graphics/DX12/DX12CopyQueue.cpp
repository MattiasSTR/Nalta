#include "npch.h"
#include "Nalta/Graphics/DX12/DX12CopyQueue.h"
#include "Nalta/Graphics/DX12/DX12Util.h"

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Nalta::Graphics
{
    struct DX12CopyQueue::Impl
    {
        ComPtr<ID3D12CommandQueue>          queue;
        ComPtr<ID3D12CommandAllocator>      allocator;
        ComPtr<ID3D12GraphicsCommandList7>  commandList;
        ComPtr<ID3D12Fence>                 fence;
        HANDLE                              fenceEvent{ nullptr };
        uint64_t                            fenceValue{ 0 };
    };

    DX12CopyQueue::DX12CopyQueue()
        : myImpl(std::make_unique<Impl>())
    {}

    DX12CopyQueue::~DX12CopyQueue() = default;

    void DX12CopyQueue::Initialize(ID3D12Device10* aDevice) const
    {
        // Create copy queue
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type     = D3D12_COMMAND_LIST_TYPE_COPY;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;

        if (FAILED(aDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&myImpl->queue))))
        {
            NL_FATAL(GCoreLogger, "DX12CopyQueue: failed to create command queue");
        }

        DX12_SET_NAME(myImpl->queue.Get(), "Copy Queue");

        // Create allocator
        if (FAILED(aDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_COPY,
            IID_PPV_ARGS(&myImpl->allocator))))
        {
            NL_FATAL(GCoreLogger, "DX12CopyQueue: failed to create command allocator");
        }

        DX12_SET_NAME(myImpl->allocator.Get(), "Copy Command Allocator");

        // Create command list in closed state
        if (FAILED(aDevice->CreateCommandList1(
            0,
            D3D12_COMMAND_LIST_TYPE_COPY,
            D3D12_COMMAND_LIST_FLAG_NONE,
            IID_PPV_ARGS(&myImpl->commandList))))
        {
            NL_FATAL(GCoreLogger, "DX12CopyQueue: failed to create command list");
        }

        DX12_SET_NAME(myImpl->commandList.Get(), "Copy Command List");

        // Create fence
        if (FAILED(aDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&myImpl->fence))))
        {
            NL_FATAL(GCoreLogger, "DX12CopyQueue: failed to create fence");
        }

        DX12_SET_NAME(myImpl->fence.Get(), "Copy Fence");

        myImpl->fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!myImpl->fenceEvent)
        {
            NL_FATAL(GCoreLogger, "DX12CopyQueue: failed to create fence event");
        }

        NL_TRACE(GCoreLogger, "DX12CopyQueue: initialized");
    }

    void DX12CopyQueue::Shutdown() const
    {
        WaitForCompletion();

        if (myImpl->fenceEvent)
        {
            CloseHandle(myImpl->fenceEvent);
            myImpl->fenceEvent = nullptr;
        }

        myImpl->fence.Reset();
        myImpl->commandList.Reset();
        myImpl->allocator.Reset();
        myImpl->queue.Reset();

        NL_TRACE(GCoreLogger, "DX12CopyQueue: shutdown");
    }

    void DX12CopyQueue::Begin() const
    {
        if (FAILED(myImpl->allocator->Reset()))
        {
            NL_FATAL(GCoreLogger, "DX12CopyQueue: failed to reset allocator");
        }

        if (FAILED(myImpl->commandList->Reset(myImpl->allocator.Get(), nullptr)))
        {
            NL_FATAL(GCoreLogger, "DX12CopyQueue: failed to reset command list");
        }
    }

    void DX12CopyQueue::End() const
    {
        if (FAILED(myImpl->commandList->Close()))
        {
            NL_FATAL(GCoreLogger, "DX12CopyQueue: failed to close command list");
        }
    }

    void DX12CopyQueue::Execute() const
    {
        ID3D12CommandList* lists[]{ myImpl->commandList.Get() };
        myImpl->queue->ExecuteCommandLists(1, lists);

        ++myImpl->fenceValue;
        if (FAILED(myImpl->queue->Signal(myImpl->fence.Get(), myImpl->fenceValue)))
        {
            NL_FATAL(GCoreLogger, "DX12CopyQueue: failed to signal fence");
        }
    }

    void DX12CopyQueue::WaitForCompletion() const
    {
        if (myImpl->fence->GetCompletedValue() < myImpl->fenceValue)
        {
            if (FAILED(myImpl->fence->SetEventOnCompletion(myImpl->fenceValue, myImpl->fenceEvent)))
            {
                NL_FATAL(GCoreLogger, "DX12CopyQueue: failed to set fence event");
            }
            WaitForSingleObject(myImpl->fenceEvent, INFINITE);
        }
    }

    void DX12CopyQueue::ExecuteAndWait() const
    {
        Execute();
        WaitForCompletion();
    }

    ID3D12CommandQueue* DX12CopyQueue::GetQueue() const
    {
        return myImpl->queue.Get();
    }

    ID3D12GraphicsCommandList7* DX12CopyQueue::GetCommandList() const
    {
        return myImpl->commandList.Get();
    }
}