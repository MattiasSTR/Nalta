#include "npch.h"
#include "Nalta/Graphics/DX12/DX12RenderSurface.h"
#include "Nalta/Graphics/DX12/DX12Device.h"
#include "Nalta/Platform/IWindow.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Nalta::Graphics
{
    namespace
    {
        constexpr DXGI_FORMAT BACKBUFFER_FORMAT{ DXGI_FORMAT_R8G8B8A8_UNORM };
        constexpr uint32_t MAX_BACKBUFFERS{ 3 };
    }
    
    struct DX12RenderSurface::Impl
    {
        ComPtr<IDXGISwapChain4> swapChain;
        ComPtr<ID3D12Resource> backbuffers[MAX_BACKBUFFERS];
        ComPtr<ID3D12DescriptorHeap> rtvHeap;
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[MAX_BACKBUFFERS]{};
        uint32_t rtvDescriptorSize{ 0 };
        uint32_t bufferCount{ 2 };

        IDXGIFactory7* factory{ nullptr };
        ID3D12Device10* device { nullptr };
        ID3D12CommandQueue* queue  { nullptr };
    };
    
    DX12RenderSurface::DX12RenderSurface(const RenderSurfaceDesc& aDesc, DX12Device* aDevice)
        : myImpl  (std::make_unique<Impl>()),
        myDevice(aDevice),
        myWindow(aDesc.window),
        myWidth (aDesc.window->GetWidth()),
        myHeight(aDesc.window->GetHeight())
    {
        N_ASSERT(aDesc.bufferCount >= 2 && aDesc.bufferCount <= MAX_BACKBUFFERS, "DX12RenderSurface: bufferCount must be between 2 and 3");

        myImpl->bufferCount = aDesc.bufferCount;
        myImpl->factory = aDevice->GetFactory();
        myImpl->device = aDevice->GetDevice();
        myImpl->queue = aDevice->GetCommandQueue();

        CreateSwapChain();
        CreateRenderTargetViews();

        NL_TRACE(GCoreLogger, "DX12RenderSurface: created ({}x{})", myWidth, myHeight);
    }
    
    DX12RenderSurface::~DX12RenderSurface()
    {
        ReleaseBackbuffers();
        NL_TRACE(GCoreLogger, "DX12RenderSurface: destroyed");
    }
    
    void DX12RenderSurface::CreateSwapChain() const
    {
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width       = myWidth;
        desc.Height      = myHeight;
        desc.Format      = BACKBUFFER_FORMAT;
        desc.Stereo      = FALSE;
        desc.SampleDesc  = { 1, 0 };
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = myImpl->bufferCount;
        desc.Scaling     = DXGI_SCALING_STRETCH;
        desc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.Flags       = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        
        const HWND hwnd{ static_cast<HWND>(myWindow->GetNativeHandle()) };
        
        ComPtr<IDXGISwapChain1> swapChain1;
        if (FAILED(myImpl->factory->CreateSwapChainForHwnd(
            myImpl->queue,
            hwnd,
            &desc,
            nullptr,
            nullptr,
            &swapChain1)))
        {
            NL_FATAL(GCoreLogger, "DX12RenderSurface: failed to create swapchain");
        }
        
        // Opt out of Alt+Enter — we manage fullscreen ourselves via Win32
        myImpl->factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
        
        if (FAILED(swapChain1.As(&myImpl->swapChain)))
        {
            NL_FATAL(GCoreLogger, "DX12RenderSurface: failed to query IDXGISwapChain4");
        }
    }
    
    void DX12RenderSurface::CreateRenderTargetViews() const
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapDesc.NumDescriptors = myImpl->bufferCount;
        heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        if (FAILED(myImpl->device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&myImpl->rtvHeap))))
        {
            NL_FATAL(GCoreLogger, "DX12RenderSurface: failed to create RTV descriptor heap");
        }

        myImpl->rtvDescriptorSize = myImpl->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        
        D3D12_CPU_DESCRIPTOR_HANDLE handle{ myImpl->rtvHeap->GetCPUDescriptorHandleForHeapStart() };
        
        for (uint32_t i{ 0 }; i < myImpl->bufferCount; ++i)
        {
            if (FAILED(myImpl->swapChain->GetBuffer(i, IID_PPV_ARGS(&myImpl->backbuffers[i]))))
            {
                NL_FATAL(GCoreLogger, "DX12RenderSurface: failed to get backbuffer {}", i);
            }

            myImpl->device->CreateRenderTargetView(myImpl->backbuffers[i].Get(), nullptr, handle);
            myImpl->rtvHandles[i] = handle;
            handle.ptr += myImpl->rtvDescriptorSize;
        }
    }
    
    void DX12RenderSurface::ReleaseBackbuffers() const
    {
        for (uint32_t i{ 0 }; i < myImpl->bufferCount; ++i)
        {
            myImpl->backbuffers[i].Reset();
        }
    }
    
    void DX12RenderSurface::Resize(uint32_t aWidth, uint32_t aHeight)
    {
        if (myWidth == aWidth && myHeight == aHeight) return;

        myWidth  = aWidth;
        myHeight = aHeight;

        ReleaseBackbuffers();

        if (FAILED(myImpl->swapChain->ResizeBuffers(
            myImpl->bufferCount,
            aWidth, aHeight,
            BACKBUFFER_FORMAT,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)))
        {
            NL_FATAL(GCoreLogger, "DX12RenderSurface: failed to resize swapchain");
        }

        CreateRenderTargetViews();
        NL_TRACE(GCoreLogger, "DX12RenderSurface: resized to {}x{}", aWidth, aHeight);
    }
    
    void DX12RenderSurface::Present(const uint32_t aSyncInterval) const
    {
        if (FAILED(myImpl->swapChain->Present(aSyncInterval, 0)))
        {
            NL_FATAL(GCoreLogger, "DX12RenderSurface: present failed");
        }
    }

    uint32_t     DX12RenderSurface::GetWidth()  const { return myWidth; }
    uint32_t     DX12RenderSurface::GetHeight() const { return myHeight; }
    WindowHandle DX12RenderSurface::GetWindow() const { return myWindow; }
}