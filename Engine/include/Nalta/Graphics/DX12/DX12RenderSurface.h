#pragma once
#include "Nalta/Graphics/RenderSurface.h"
#include "Nalta/Graphics/RenderSurfaceDesc.h"

#include <memory>

namespace Nalta::Graphics
{
    class DX12Device;

    class DX12RenderSurface final : public RenderSurface
    {
    public:
        DX12RenderSurface(const RenderSurfaceDesc& aDesc, DX12Device* aDevice);
        ~DX12RenderSurface() override;

        void Resize(uint32_t aWidth, uint32_t aHeight) override;

        [[nodiscard]] uint32_t GetWidth()  const override;
        [[nodiscard]] uint32_t GetHeight() const override;
        [[nodiscard]] WindowHandle GetWindow() const override;

        void Present(uint32_t aSyncInterval = 1) const;

    private:
        void CreateSwapChain() const;
        void CreateRenderTargetViews() const;
        void ReleaseBackbuffers() const;

        struct Impl;
        std::unique_ptr<Impl> myImpl;

        DX12Device* myDevice{ nullptr };
        WindowHandle myWindow;
        uint32_t myWidth { 0 };
        uint32_t myHeight{ 0 };
    };
}