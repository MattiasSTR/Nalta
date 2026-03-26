#include "npch.h"
#include "Nalta/Graphics/DX12/DX12ConstantBuffer.h"
#include "Nalta/Graphics/DX12/DX12Device.h"
#include "Nalta/Graphics/DX12/DX12Util.h"

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Nalta::Graphics
{
    namespace
    {
        // DX12 requires constant buffers to be 256-byte aligned
        constexpr uint32_t CBV_ALIGNMENT{ 256 };

        uint32_t AlignToCBV(const uint32_t aSize)
        {
            return (aSize + CBV_ALIGNMENT - 1) & ~(CBV_ALIGNMENT - 1);
        }
    }

    struct DX12ConstantBuffer::Impl
    {
        ComPtr<ID3D12Resource> resource;
        uint8_t*               mappedPtr{ nullptr };
    };

    DX12ConstantBuffer::DX12ConstantBuffer(const uint32_t aSize, DX12Device* aDevice)
        : myImpl(std::make_unique<Impl>())
        , myDevice(aDevice)
        , mySize(aSize)
        , myAlignedSize(AlignToCBV(aSize))
    {
        NL_SCOPE_CORE("DX12ConstantBuffer");
        const uint32_t framesInFlight{ aDevice->GetFramesInFlight() };
        const uint64_t totalSize{ static_cast<uint64_t>(myAlignedSize) * framesInFlight };

        constexpr D3D12_HEAP_PROPERTIES uploadHeap
        {
            .Type                 = D3D12_HEAP_TYPE_UPLOAD,
            .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN
        };

        const D3D12_RESOURCE_DESC bufferDesc
        {
            .Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment        = 0,
            .Width            = totalSize,
            .Height           = 1,
            .DepthOrArraySize = 1,
            .MipLevels        = 1,
            .Format           = DXGI_FORMAT_UNKNOWN,
            .SampleDesc       = { 1, 0 },
            .Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags            = D3D12_RESOURCE_FLAG_NONE
        };

        if (FAILED(aDevice->GetDevice()->CreateCommittedResource(
            &uploadHeap,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&myImpl->resource))))
        {
            NL_FATAL(GCoreLogger, "failed to create resource");
        }

        DX12_SET_NAME(myImpl->resource.Get(), "Constant Buffer");

        // Persist map — valid for lifetime of the resource on upload heap
        constexpr D3D12_RANGE readRange{ 0, 0 };
        if (FAILED(myImpl->resource->Map(0, &readRange,
            reinterpret_cast<void**>(&myImpl->mappedPtr))))
        {
            NL_FATAL(GCoreLogger, "failed to map resource");
        }

        NL_TRACE(GCoreLogger, "created ({} bytes x {} frames)", myAlignedSize, framesInFlight);
    }

    DX12ConstantBuffer::~DX12ConstantBuffer()
    {
        if (myImpl->resource && myImpl->mappedPtr)
        {
            myImpl->resource->Unmap(0, nullptr);
            myImpl->mappedPtr = nullptr;
        }
    }

    uint32_t DX12ConstantBuffer::GetSizeInBytes() const { return mySize; }
    bool DX12ConstantBuffer::IsValid() const { return myImpl->resource != nullptr; }

    void DX12ConstantBuffer::Update(const void* aData, const uint32_t aSize) const
    {
        NL_SCOPE_CORE("DX12ConstantBuffer");
        N_CORE_ASSERT(aData, "null data");
        N_CORE_ASSERT(aSize <= mySize, "data exceeds buffer size");

        const uint32_t frameIndex{ myDevice->GetFrameIndex() };
        const uint32_t offset{ frameIndex * myAlignedSize };

        memcpy(myImpl->mappedPtr + offset, aData, aSize);
    }

    uint64_t DX12ConstantBuffer::GetGPUAddress() const
    {
        NL_SCOPE_CORE("DX12ConstantBuffer");
        N_CORE_ASSERT(IsValid(), "resource not valid");

        const uint32_t frameIndex{ myDevice->GetFrameIndex() };
        const uint32_t offset{ frameIndex * myAlignedSize };

        return myImpl->resource->GetGPUVirtualAddress() + offset;
    }
}