#pragma once
#include <cstdint>
#include <string>

namespace Nalta
{
    enum class WindowMode : uint8_t
    {
        Windowed,
        WindowedFullscreen,
        Fullscreen
    };
    
    class IWindow
    {
    public:
        virtual ~IWindow() = default;

        virtual void Show() = 0;
        virtual void Hide() = 0;
        virtual void Resize(uint32_t aWidth, uint32_t aHeight) = 0;
        virtual void SetWindowMode(WindowMode aMode) = 0;
        virtual void SetTitle(const std::string& aTitle) = 0;

        [[nodiscard]] virtual uint32_t GetWidth()        const = 0;
        [[nodiscard]] virtual uint32_t GetHeight()       const = 0;
        [[nodiscard]] virtual void* GetNativeHandle() const = 0;
        [[nodiscard]] virtual bool IsMainWindow()    const = 0;
        [[nodiscard]] virtual bool IsVisible()       const = 0;
        [[nodiscard]] virtual float GetDPIScale()     const = 0;
    };
}