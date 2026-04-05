#pragma once
#include "Nalta/RHI/Types/RHIDescs.h"
#include "Nalta/RHI/D3D12/Common/D3D12Common.h"
#include "Nalta/RHI/D3D12/Resources/D3D12Texture.h"

#include <array>

namespace Nalta::RHI::D3D12
{
    class Device;
    class GraphicsContext;

    class RenderSurface final
    {
    public:
        RenderSurface(Device& aDevice, const RenderSurfaceDesc& aDesc);
        ~RenderSurface();

        RenderSurface(const RenderSurface&) = delete;
        RenderSurface& operator=(const RenderSurface&) = delete;
        RenderSurface(RenderSurface&&) = delete;
        RenderSurface& operator=(RenderSurface&&) = delete;

        void Present(bool aVSync = false);
        void SetAsRenderTarget(GraphicsContext& aContext);
        void EndRenderTarget(GraphicsContext& aContext);
        void Resize(uint32_t aWidth, uint32_t aHeight);
        void Clear(GraphicsContext& aContext, const float aClearColor[4] = nullptr);

        [[nodiscard]] TextureResource& GetCurrentBackBuffer();
        [[nodiscard]] uint32_t GetWidth() const { return myWidth; }
        [[nodiscard]] uint32_t GetHeight() const { return myHeight; }
        [[nodiscard]] uint32_t GetBufferCount() const { return myBufferCount; }

    private:
        void CreateSwapChain(HWND aWindow);
        void CreateBackBuffers();
        void ReleaseBackBuffers();

        Device& myDevice;
        IDXGISwapChain4* mySwapChain{ nullptr };
        uint32_t myWidth{ 0 };
        uint32_t myHeight{ 0 };
        uint32_t myBufferCount{ 0 };

        // Backbuffers are TextureResource but allocation is null - swap chain owns the memory
        std::array<std::unique_ptr<TextureResource>, BACK_BUFFER_COUNT> myBackBuffers;
    };
}