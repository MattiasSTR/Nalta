#pragma once
#include <cstdint>

namespace Nalta::Graphics
{
    class IDepthBuffer
    {
    public:
        virtual ~IDepthBuffer() = default;

        virtual void Resize(uint32_t aWidth, uint32_t aHeight) = 0;
        
        virtual void Clear() = 0;

        [[nodiscard]] virtual uint32_t GetWidth() const = 0;
        [[nodiscard]] virtual uint32_t GetHeight() const = 0;
        [[nodiscard]] virtual bool IsValid() const = 0;
    };
}