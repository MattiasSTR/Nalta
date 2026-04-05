#pragma once
#include <cstdint>
#include <string>
#include "Nalta/Util/SlotKey.h"

namespace Nalta
{
    enum class WindowMode : uint8_t
    {
        Windowed,
        WindowedFullscreen,
        Fullscreen
    };
    
    struct WindowDesc
    {
        uint32_t width          { 1280 };
        uint32_t height         { 720 };
        std::string caption     { "Nalta" };
        WindowMode windowMode   { WindowMode::Windowed };
        bool resizable          { true };
        bool isMainWindow       { false };
    };
    
    struct WindowKey : SlotKey { using SlotKey::SlotKey; };
    
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