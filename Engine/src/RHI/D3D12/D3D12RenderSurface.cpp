#include "npch.h"
#include "Nalta/RHI/D3D12/D3D12RenderSurface.h"
#include "Nalta/RHI/D3D12/D3D12Device.h"
#include "Nalta/RHI/D3D12/Contexts/D3D12GraphicsContext.h"
#include "Nalta/RHI/D3D12/Common/D3D12Queue.h"

namespace Nalta::RHI::D3D12
{
    namespace
    {
        constexpr DXGI_FORMAT SwapChainFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
        constexpr DXGI_FORMAT RTVFormat{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };
    }
    
    RenderSurface::RenderSurface(Device& aDevice, const RenderSurfaceDesc& aDesc)
        : myDevice     (aDevice)
        , myWidth      (aDesc.width)
        , myHeight     (aDesc.height)
        , myBufferCount(aDesc.bufferCount)
    {
        N_CORE_ASSERT(aDesc.window != nullptr, "Window handle must be valid");
        N_CORE_ASSERT(aDesc.width > 0 && aDesc.height > 0, "Surface dimensions must be non-zero");
        N_CORE_ASSERT(aDesc.bufferCount >= 2 && aDesc.bufferCount <= BACK_BUFFER_COUNT, "Buffer count must be between 2 and BACK_BUFFER_COUNT");

        HWND hwnd{ static_cast<HWND>(aDesc.window) };
        CreateSwapChain(hwnd);
        CreateBackBuffers();

        NL_TRACE(GCoreLogger, "render surface created [{}x{} buffers:{}]", myWidth, myHeight, myBufferCount);
    }
    
    RenderSurface::~RenderSurface()
    {
        ReleaseBackBuffers();
        SafeRelease(mySwapChain);
        NL_TRACE(GCoreLogger, "render surface destroyed");
    }
    
    void RenderSurface::CreateSwapChain(HWND aWindow)
    {
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width       = myWidth;
        desc.Height      = myHeight;
        desc.Format      = SwapChainFormat;
        desc.Stereo      = FALSE;
        desc.SampleDesc  = { 1, 0 };
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = myBufferCount;
        desc.Scaling     = DXGI_SCALING_NONE;
        desc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.Flags       = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        IDXGISwapChain1* swapChain1{ nullptr };
        NL_DX_VERIFY(myDevice.GetDXGIFactory()->CreateSwapChainForHwnd(
            myDevice.GetQueue(QueueType::Graphics).GetCommandQueue(),
            aWindow,
            &desc,
            nullptr,
            nullptr,
            &swapChain1));

        // Opt out of Alt+Enter — we manage fullscreen ourselves
        myDevice.GetDXGIFactory()->MakeWindowAssociation(aWindow, DXGI_MWA_NO_ALT_ENTER);

        NL_DX_VERIFY(swapChain1->QueryInterface(IID_PPV_ARGS(&mySwapChain)));
        SafeRelease(swapChain1);

    }
    
    void RenderSurface::CreateBackBuffers()
    {
        for (uint32_t i{ 0 }; i < myBufferCount; ++i)
        {
            auto backBuffer{ std::make_unique<TextureResource>() };

            ID3D12Resource* resource{ nullptr };
            NL_DX_VERIFY(mySwapChain->GetBuffer(i, IID_PPV_ARGS(&resource)));

            backBuffer->resource   = resource;
            backBuffer->allocation = nullptr; // swap chain owns the memory
            backBuffer->state      = D3D12_RESOURCE_STATE_PRESENT;
            backBuffer->type       = ResourceType::Texture;
            backBuffer->isReady    = true;

            // Allocate RTV from device heap
            backBuffer->RTVDescriptor = myDevice.GetRTVHeap().Allocate();

            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
            rtvDesc.Format        = RTVFormat;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            myDevice.GetD3D12Device()->CreateRenderTargetView(resource, &rtvDesc, backBuffer->RTVDescriptor.CPUHandle);

            const std::wstring name{ L"BackBuffer[" + std::to_wstring(i) + L"]" };
            N_D3D12_SET_NAME_W(resource, name.c_str());

            myBackBuffers[i] = std::move(backBuffer);
        }
    }
    
    void RenderSurface::ReleaseBackBuffers()
    {
        for (uint32_t i{ 0 }; i < myBufferCount; ++i)
        {
            if (myBackBuffers[i])
            {
                if (myBackBuffers[i]->RTVDescriptor.IsValid())
                {
                    myDevice.GetRTVHeap().Free(myBackBuffers[i]->RTVDescriptor);
                }

                // Don't release allocation - null for backbuffers
                // Release the resource ref we hold from GetBuffer
                SafeRelease(myBackBuffers[i]->resource);
                myBackBuffers[i].reset();
            }
        }
    }

    void RenderSurface::Present(bool aVSync)
    {
        const UINT syncInterval{ aVSync ? 1u : 0u };
        const UINT flags{ aVSync ? 0u : DXGI_PRESENT_ALLOW_TEARING };

        NL_DX_VERIFY(mySwapChain->Present(syncInterval, flags));
    }

    void RenderSurface::SetAsRenderTarget(GraphicsContext& aContext)
    {
        TextureResource& backBuffer{ GetCurrentBackBuffer() };

        // Transition from present to render target
        aContext.AddBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
        aContext.FlushBarriers();

        const TextureResource* targets[]{ &backBuffer };
        aContext.SetRenderTargets(targets, nullptr);
        aContext.SetViewport(myWidth, myHeight);
        aContext.SetScissor(myWidth, myHeight);
    }

    void RenderSurface::EndRenderTarget(GraphicsContext& aContext)
    {
        TextureResource& backBuffer{ GetCurrentBackBuffer() };

        // Transition back to present
        aContext.AddBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT);
        aContext.FlushBarriers();
    }

    void RenderSurface::Resize(uint32_t aWidth, uint32_t aHeight)
    {
        if (myWidth == aWidth && myHeight == aHeight)
        {
            return;
        }

        myWidth  = aWidth;
        myHeight = aHeight;

        ReleaseBackBuffers();

        NL_DX_VERIFY(mySwapChain->ResizeBuffers(
            myBufferCount,
            aWidth, aHeight,
            SwapChainFormat,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING));

        CreateBackBuffers();

        NL_TRACE(GCoreLogger, "render surface resized [{}x{}]", aWidth, aHeight);
    }

    void RenderSurface::Clear(GraphicsContext& aContext, const float aClearColor[4])
    {
        static constexpr float defaultColor[4]{ 0.0f, 0.0f, 0.0f, 1.0f };

        aContext.ClearRenderTarget(GetCurrentBackBuffer(), aClearColor ? aClearColor : defaultColor);
    }

    TextureResource& RenderSurface::GetCurrentBackBuffer()
    {
        return *myBackBuffers[mySwapChain->GetCurrentBackBufferIndex()];
    }
}
