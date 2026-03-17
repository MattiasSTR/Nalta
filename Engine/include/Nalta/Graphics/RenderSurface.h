#pragma once
#include "Nalta/Platform/WindowHandle.h"

#include <cstdint>

namespace Nalta::Graphics
{
    class RenderSurface
    {
    public:
        virtual ~RenderSurface() = default;

        virtual void Resize(uint32_t aWidth, uint32_t aHeight) = 0;

        [[nodiscard]] virtual uint32_t     GetWidth()  const = 0;
        [[nodiscard]] virtual uint32_t     GetHeight() const = 0;
        [[nodiscard]] virtual WindowHandle GetWindow() const = 0;
    };
}
