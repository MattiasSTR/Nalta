#pragma once
#include <cstdint>

namespace Nalta
{
    class IWindow
    {
    public:
        virtual ~IWindow() = default;

        [[nodiscard]] virtual bool IsClosed() const = 0;

        [[nodiscard]] virtual uint32_t GetWidth() const = 0;
        [[nodiscard]] virtual uint32_t GetHeight() const = 0;
        virtual void Resize(uint32_t aWidth, uint32_t aHeight) = 0;

        virtual void SetFullscreen(bool aFullscreen) = 0;
        [[nodiscard]] virtual bool IsFullscreen() const = 0;

        virtual void SetCaption(const std::string& aCaption) = 0;

        [[nodiscard]] virtual void* GetNativeHandle() const = 0;
    };
}