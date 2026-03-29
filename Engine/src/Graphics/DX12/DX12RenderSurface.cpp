#include "npch.h"
#include "Nalta/Graphics/DX12/DX12RenderSurface.h"

#include "Nalta/Graphics/DX12/DX12DepthBuffer.h"
#include "Nalta/Graphics/DX12/DX12Device.h"
#include "Nalta/Graphics/DX12/DX12Util.h"
#include "Nalta/Platform/IWindow.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Nalta::Graphics
{
    namespace
    {
        // Swap chain format - must be UNORM
        constexpr DXGI_FORMAT SWAPCHAIN_FORMAT{ DXGI_FORMAT_R8G8B8A8_UNORM };

        // RTV format - sRGB for correct gamma output
        constexpr DXGI_FORMAT RTV_FORMAT{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };
        
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
        NL_SCOPE_CORE("DX12RenderSurface");
        N_CORE_ASSERT(aDesc.bufferCount >= 2 && aDesc.bufferCount <= MAX_BACKBUFFERS, "bufferCount must be between 2 and 3");

        myImpl->bufferCount = aDesc.bufferCount;
        myImpl->factory = aDevice->GetFactory();
        myImpl->device = aDevice->GetDevice();
        myImpl->queue = aDevice->GetCommandQueue();

        CreateSwapChain();
        CreateRenderTargetViews();

        NL_TRACE(GCoreLogger, "created ({}x{})", myWidth, myHeight);
    }
    
    DX12RenderSurface::~DX12RenderSurface()
    {
        NL_SCOPE_CORE("DX12RenderSurface");
        ReleaseBackbuffers();
        NL_TRACE(GCoreLogger, "destroyed");
    }
    
    void DX12RenderSurface::CreateSwapChain() const
    {
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width       = myWidth;
        desc.Height      = myHeight;
        desc.Format      = SWAPCHAIN_FORMAT;
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
            NL_FATAL(GCoreLogger, "failed to create swapchain");
        }
        
        // Opt out of Alt+Enter — we manage fullscreen ourselves via Win32
        myImpl->factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
        
        if (FAILED(swapChain1.As(&myImpl->swapChain)))
        {
            NL_FATAL(GCoreLogger, "failed to query IDXGISwapChain4");
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
            NL_FATAL(GCoreLogger, "failed to create RTV descriptor heap");
        }

        myImpl->rtvDescriptorSize = myImpl->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        
        D3D12_CPU_DESCRIPTOR_HANDLE handle{ myImpl->rtvHeap->GetCPUDescriptorHandleForHeapStart() };
        
        DX12_SET_NAME(myImpl->rtvHeap.Get(), "RTV Descriptor Heap");
        
        for (uint32_t i{ 0 }; i < myImpl->bufferCount; ++i)
        {
            if (FAILED(myImpl->swapChain->GetBuffer(i, IID_PPV_ARGS(&myImpl->backbuffers[i]))))
            {
                NL_FATAL(GCoreLogger, "failed to get backbuffer {}", i);
            }

            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
            rtvDesc.Format        = RTV_FORMAT;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            myImpl->device->CreateRenderTargetView(myImpl->backbuffers[i].Get(), &rtvDesc, handle);
            myImpl->rtvHandles[i] = handle;
            handle.ptr += myImpl->rtvDescriptorSize;
            
            const std::wstring name{ L"Backbuffer " + std::to_wstring(i) };
            DX12_SET_NAME_W(myImpl->backbuffers[i].Get(), name.c_str());
        }
    }
    
    void DX12RenderSurface::ReleaseBackbuffers() const
    {
        for (uint32_t i{ 0 }; i < myImpl->bufferCount; ++i)
        {
            myImpl->backbuffers[i].Reset();
        }
    }

    uint32_t DX12RenderSurface::GetCurrentBackBufferIndex() const
    {
        return myImpl->swapChain->GetCurrentBackBufferIndex();
    }

    void DX12RenderSurface::Resize(uint32_t aWidth, uint32_t aHeight)
    {
        if (myWidth == aWidth && myHeight == aHeight)
        {
            return;
        }
        NL_SCOPE_CORE("DX12RenderSurface");

        myWidth  = aWidth;
        myHeight = aHeight;

        ReleaseBackbuffers();

        if (FAILED(myImpl->swapChain->ResizeBuffers(
            myImpl->bufferCount,
            aWidth, aHeight,
            SWAPCHAIN_FORMAT,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)))
        {
            NL_FATAL(GCoreLogger, "failed to resize swapchain");
        }

        CreateRenderTargetViews();
        NL_TRACE(GCoreLogger, "resized to {}x{}", aWidth, aHeight);
    }
    
    void DX12RenderSurface::Present(const uint32_t aSyncInterval)
    {
        if (FAILED(myImpl->swapChain->Present(aSyncInterval, 0)))
        {
            NL_FATAL(GCoreLogger, "present failed");
        }
    }

    void DX12RenderSurface::SetAsRenderTarget(DepthBufferHandle aDepthBuffer)
    {
        auto* cmdList{ myDevice->GetCommandList() };
        const uint32_t backBufferIndex{ GetCurrentBackBufferIndex() };
        
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = myImpl->backbuffers[backBufferIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{};
        const D3D12_CPU_DESCRIPTOR_HANDLE* dsv{ nullptr };

        if (aDepthBuffer.IsValid())
        {
            dsvHandle = static_cast<DX12DepthBuffer*>(aDepthBuffer.Get())->GetDSV();
            dsv = &dsvHandle;
        }
        
        cmdList->OMSetRenderTargets(
            1, 
            &myImpl->rtvHandles[backBufferIndex], 
            FALSE,
            dsv);

        D3D12_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width    = static_cast<float>(myWidth);
        viewport.Height   = static_cast<float>(myHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        D3D12_RECT scissor{};
        scissor.left   = 0;
        scissor.top    = 0;
        scissor.right  = static_cast<LONG>(myWidth);
        scissor.bottom = static_cast<LONG>(myHeight);

        cmdList->RSSetViewports(1, &viewport);
        cmdList->RSSetScissorRects(1, &scissor);
    }

    void DX12RenderSurface::EndRenderTarget()
    {
        auto* cmdList{ myDevice->GetCommandList() };
        const uint32_t backBufferIndex{ GetCurrentBackBufferIndex() };
        
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = myImpl->backbuffers[backBufferIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    void DX12RenderSurface::Clear(const float aClearColor[4])
    {
        auto* cmdList{ myDevice->GetCommandList() };
        const uint32_t backBufferIndex{ GetCurrentBackBufferIndex() };
        cmdList->ClearRenderTargetView(myImpl->rtvHandles[backBufferIndex], aClearColor, 0, nullptr);
    }
    
    uint32_t DX12RenderSurface::GetWidth() const { return myWidth; }
    uint32_t DX12RenderSurface::GetHeight() const { return myHeight; }
    WindowHandle DX12RenderSurface::GetWindow() const { return myWindow; }
}
