#pragma once
#include "Nalta/Graphics/RenderResources/DepthBufferHandle.h"
#include "Nalta/Platform/WindowHandle.h"

#include <cstdint>

namespace Nalta::Graphics
{
    class IRenderSurface
    {
    public:
        virtual ~IRenderSurface() = default;

        virtual void Resize(uint32_t aWidth, uint32_t aHeight) = 0;
        virtual void Clear(const float aClearColor[4]) = 0;
        virtual void Present(uint32_t aSyncInterval = 1) = 0;
        
        virtual void SetAsRenderTarget(DepthBufferHandle aDepthBuffer = DepthBufferHandle{}) = 0;
        virtual void EndRenderTarget() = 0;

        [[nodiscard]] virtual uint32_t GetWidth()  const = 0;
        [[nodiscard]] virtual uint32_t GetHeight() const = 0;
        [[nodiscard]] virtual WindowHandle GetWindow() const = 0;
    };
}
