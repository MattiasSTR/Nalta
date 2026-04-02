#pragma once
#include "Nalta/RHI/D3D12/D3D12Common.h"
#include "Nalta/RHI/D3D12/D3D12Queue.h"

#include <array>
#include <D3D12MemAlloc.h>

namespace Nalta::RHI::D3D12
{
    class Device final
    {
    public:
        Device();
        ~Device();

        // Non-copyable, non-movable. There is one device.
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;
        Device(Device&&) = delete;
        Device& operator=(Device&&) = delete;
        
        void BeginFrame();
        void EndFrame();
        
        ID3D12Device10* GetD3D12Device() const { return myDevice; }
        IDXGIFactory7* GetDXGIFactory() const { return myFactory; }
        IDxcUtils* GetDxcUtils() const { return myDxcUtils; }
        D3D12MA::Allocator* GetAllocator() const { return myAllocator; }
        Queue& GetQueue(QueueType aType) { return *myQueues[static_cast<size_t>(aType)]; }
        
    private:
        void InitDebugLayer();
        void InitInfoQueue();
        void SelectAdapter();
        void InitAllocator();
        void CheckFeatureSupport() const;
        
        IDXGIFactory7* myFactory{ nullptr };
        IDXGIAdapter4* myAdapter{ nullptr };
        ID3D12Device10* myDevice{ nullptr };
        IDxcUtils* myDxcUtils{ nullptr };
        D3D12MA::Allocator* myAllocator{ nullptr };
        
#ifndef N_SHIPPING
        ID3D12Debug6* myDebugController{ nullptr };
        DWORD myCallbackCookie{ 0 };
#endif
        
        std::array<std::unique_ptr<Queue>, static_cast<size_t>(QueueType::Count)> myQueues;
    };
}
