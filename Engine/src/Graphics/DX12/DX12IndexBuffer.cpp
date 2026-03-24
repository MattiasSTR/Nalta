#include "npch.h"
#include "Nalta/Graphics/DX12/DX12IndexBuffer.h"
#include "Nalta/Graphics/DX12/DX12Util.h"

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Nalta::Graphics
{
    struct DX12IndexBuffer::Impl
    {
        ComPtr<ID3D12Resource> resource;
    };

    DX12IndexBuffer::DX12IndexBuffer(const uint32_t aCount, const IndexFormat aFormat)
        : myImpl  (std::make_unique<Impl>())
        , myCount (aCount)
        , myFormat(aFormat)
    {}

    DX12IndexBuffer::~DX12IndexBuffer() = default;

    // IBuffer
    uint32_t DX12IndexBuffer::GetSizeInBytes() const
    {
        const uint32_t stride{ myFormat == IndexFormat::Uint16 ? sizeof(uint16_t) : sizeof(uint32_t) };
        return myCount * stride;
    }

    bool DX12IndexBuffer::IsValid() const { return myCount > 0; }

    // IIndexBuffer
    uint32_t DX12IndexBuffer::GetIndexCount() const { return myCount; }
    IndexFormat DX12IndexBuffer::GetFormat() const { return myFormat; }

    // IGPUResource
    bool DX12IndexBuffer::IsReady() const { return myImpl->resource != nullptr; }

    uint64_t DX12IndexBuffer::GetGPUAddress() const
    {
        N_CORE_ASSERT(IsReady(), "DX12IndexBuffer: resource not yet uploaded");
        return myImpl->resource->GetGPUVirtualAddress();
    }

    // IDX12GPUResource
    void DX12IndexBuffer::SetResource(ID3D12Resource* aResource)
    {
        myImpl->resource = aResource;
        DX12_SET_NAME(myImpl->resource.Get(), "Index Buffer");
    }

    // DX12 specific
    ID3D12Resource* DX12IndexBuffer::GetResource() const { return myImpl->resource.Get(); }
}