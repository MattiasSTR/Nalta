#pragma once
#include "Nalta/Graphics/RenderSurface.h"

#include <memory>

namespace Nalta::Graphics
{
    class DX12RenderSurface : public RenderSurface
    {
    public:
        DX12RenderSurface() = default;
        ~DX12RenderSurface() override = default;

        void Initialize(std::shared_ptr<IWindow> aWindow) override;

        void Shutdown() override;

        void Present() override;

        uint32_t GetWidth() const override;
        uint32_t GetHeight() const override;

    private:
        std::shared_ptr<IWindow> myWindow;
    };
}