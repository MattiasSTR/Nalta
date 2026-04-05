#pragma once
#include "Nalta/RHI/D3D12/Common/D3D12Common.h"
#include "Nalta/RHI/D3D12/Common/D3D12Resource.h"
#include "Nalta/RHI/Types/RHIConstants.h"

namespace Nalta::RHI::D3D12
{
    class Device;

    class Context
    {
    public:
        Context(Device& aDevice, D3D12_COMMAND_LIST_TYPE aType);
        virtual ~Context();

        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context&&) = delete;

        virtual void Reset();
        void AddBarrier(Resource& aResource, D3D12_RESOURCE_STATES aNewState);
        void FlushBarriers();
        
        void Close();

        void CopyResource(Resource& aDst, const Resource& aSrc);
        void CopyBufferRegion(Resource& aDst, uint64_t aDstOffset, const Resource& aSrc, uint64_t aSrcOffset, uint64_t aNumBytes);

        [[nodiscard]] ID3D12GraphicsCommandList6* GetCommandList() const { return myCommandList; }
        [[nodiscard]] D3D12_COMMAND_LIST_TYPE GetType() const { return myType; }

    protected:
        void BindDescriptorHeaps();

        Device& myDevice;
        D3D12_COMMAND_LIST_TYPE myType;
        ID3D12GraphicsCommandList6* myCommandList{ nullptr };

        std::array<ID3D12CommandAllocator*, FRAMES_IN_FLIGHT> myCommandAllocators{ nullptr };
        std::array<D3D12_RESOURCE_BARRIER, MAX_QUEUED_BARRIERS> myBarriers{};
        uint32_t myNumQueuedBarriers{ 0 };
        
        bool myIsRecording{ false };
    };
}
