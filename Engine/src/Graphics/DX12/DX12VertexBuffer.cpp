#include "npch.h"
#include "Nalta/Graphics/DX12/DX12VertexBuffer.h"
#include "Nalta/Graphics/DX12/DX12Util.h"

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Nalta::Graphics
{
    struct DX12VertexBuffer::Impl
    {
        ComPtr<ID3D12Resource> resource;
    };

    DX12VertexBuffer::DX12VertexBuffer(const uint32_t aStride, const uint32_t aCount)
        : myImpl  (std::make_unique<Impl>())
        , myStride(aStride)
        , myCount (aCount)
    {}

    DX12VertexBuffer::~DX12VertexBuffer() = default;

    // IBuffer
    uint32_t DX12VertexBuffer::GetSizeInBytes() const { return myStride * myCount; }
    bool DX12VertexBuffer::IsValid() const { return myStride > 0 && myCount > 0; }

    // IVertexBuffer
    uint32_t DX12VertexBuffer::GetStride() const { return myStride; }
    uint32_t DX12VertexBuffer::GetVertexCount() const { return myCount; }
    
    bool DX12VertexBuffer::IsReady() const
    {
        return myImpl->resource != nullptr;
    }

    uint64_t DX12VertexBuffer::GetGPUAddress() const
    {
        NL_SCOPE_CORE("DX12VertexBuffer");
        N_CORE_ASSERT(IsReady(), "resource not yet uploaded");
        return myImpl->resource->GetGPUVirtualAddress();
    }

    void DX12VertexBuffer::SetResource(ID3D12Resource* aResource)
    {
        myImpl->resource = aResource;
        DX12_SET_NAME(myImpl->resource.Get(), "Vertex Buffer");
    }

    ID3D12Resource* DX12VertexBuffer::GetResource() const
    {
        return myImpl->resource.Get();
    }
}