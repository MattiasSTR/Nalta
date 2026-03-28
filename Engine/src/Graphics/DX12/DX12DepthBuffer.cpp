#include "npch.h"
#include "Nalta/Graphics/DX12/DX12DepthBuffer.h"
#include "Nalta/Graphics/DX12/DX12Device.h"
#include "Nalta/Graphics/DX12/DX12Util.h"
#include "Nalta/Graphics/RenderResources/DepthBufferDesc.h"

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Nalta::Graphics
{
    namespace
    {
        // Reversed-Z — near=1.0, far=0.0
        constexpr DXGI_FORMAT DEPTH_FORMAT{ DXGI_FORMAT_D32_FLOAT };
        constexpr float DEPTH_CLEAR{ 0.0f }; // reversed-Z clears to 0
    }
    
    struct DX12DepthBuffer::Impl
    {
        ComPtr<ID3D12Resource>       resource;
        ComPtr<ID3D12DescriptorHeap> dsvHeap;
        D3D12_CPU_DESCRIPTOR_HANDLE  dsvHandle{};
    };

    DX12DepthBuffer::DX12DepthBuffer(const DepthBufferDesc& aDesc, DX12Device* aDevice)
        : myImpl  (std::make_unique<Impl>())
        , myDevice(aDevice)
        , myWidth (aDesc.width)
        , myHeight(aDesc.height)
    {
        NL_SCOPE_CORE("DX12DepthBuffer");
        
        CreateResources();
        NL_TRACE(GCoreLogger, "created ({}x{}, reversed-Z)", myWidth, myHeight);
    }
    
    DX12DepthBuffer::~DX12DepthBuffer()
    {
        ReleaseResources();
    }
    
    void DX12DepthBuffer::CreateResources() const
    {
        auto* device{ myDevice->GetDevice() };

        // Create DSV descriptor heap
        constexpr D3D12_DESCRIPTOR_HEAP_DESC heapDesc
        {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            .NumDescriptors = 1,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
        };

        if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&myImpl->dsvHeap))))
        {
            NL_FATAL(GCoreLogger, "failed to create DSV heap");
        }

        DX12_SET_NAME(myImpl->dsvHeap.Get(), "Depth Buffer DSV Heap");

        myImpl->dsvHandle = myImpl->dsvHeap->GetCPUDescriptorHandleForHeapStart();

        // Create depth resource
        constexpr D3D12_HEAP_PROPERTIES heapProps
        {
            .Type                 = D3D12_HEAP_TYPE_DEFAULT,
            .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN
        };

        const D3D12_RESOURCE_DESC resourceDesc
        {
            .Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Alignment        = 0,
            .Width            = myWidth,
            .Height           = myHeight,
            .DepthOrArraySize = 1,
            .MipLevels        = 1,
            .Format           = DEPTH_FORMAT,
            .SampleDesc       = { 1, 0 },
            .Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        };

        // Optimized clear value — reversed-Z clears to 0
        constexpr D3D12_CLEAR_VALUE clearValue
        {
            .Format       = DEPTH_FORMAT,
            .DepthStencil = { .Depth = DEPTH_CLEAR, .Stencil = 0 }
        };

        if (FAILED(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&myImpl->resource))))
        {
            NL_FATAL(GCoreLogger, "failed to create depth resource");
        }

        DX12_SET_NAME(myImpl->resource.Get(), "Depth Buffer");

        // Create DSV
        constexpr D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc
        {
            .Format        = DEPTH_FORMAT,
            .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
            .Flags         = D3D12_DSV_FLAG_NONE,
            .Texture2D     = { .MipSlice = 0 }
        };

        device->CreateDepthStencilView(
            myImpl->resource.Get(),
            &dsvDesc,
            myImpl->dsvHandle);
    }
    
    void DX12DepthBuffer::ReleaseResources() const
    {
        myImpl->resource.Reset();
        myImpl->dsvHeap.Reset();
    }
    
    void DX12DepthBuffer::Resize(const uint32_t aWidth, const uint32_t aHeight)
    {
        if (myWidth == aWidth && myHeight == aHeight)
        {
            return;
        }
        
        NL_SCOPE_CORE("DX12DepthBuffer");
        
        myWidth  = aWidth;
        myHeight = aHeight;

        ReleaseResources();
        CreateResources();

        NL_TRACE(GCoreLogger, "resized to {}x{}", aWidth, aHeight);
    }

    void DX12DepthBuffer::Clear()
    {
        myDevice->GetCommandList()->ClearDepthStencilView(
            myImpl->dsvHandle,
            D3D12_CLEAR_FLAG_DEPTH,
            DEPTH_CLEAR,
            0,
            0, nullptr);
    }

    uint32_t DX12DepthBuffer::GetWidth() const { return myWidth; }
    uint32_t DX12DepthBuffer::GetHeight() const { return myHeight; }
    bool DX12DepthBuffer::IsValid() const { return myImpl->resource != nullptr; }

    D3D12_CPU_DESCRIPTOR_HANDLE DX12DepthBuffer::GetDSV() const
    {
        return myImpl->dsvHandle;
    }
}