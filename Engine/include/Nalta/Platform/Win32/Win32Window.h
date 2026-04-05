#pragma once
#include <cstdint>
#include <string>
#include "Nalta/Platform/IWindow.h"

namespace Nalta
{
    class Win32Window final : public IWindow
    {
    public:
        explicit Win32Window(const WindowDesc& aDesc, void* aCreateParams);
        ~Win32Window() override;

        void Show() override;
        void Hide() override;
        void Resize(uint32_t aWidth, uint32_t aHeight) override;
        void SetWindowMode(WindowMode aMode) override;
        void SetTitle(const std::string& aTitle) override;

        [[nodiscard]] uint32_t GetWidth()        const override;
        [[nodiscard]] uint32_t GetHeight()        const override;
        [[nodiscard]] void* GetNativeHandle()  const override;
        [[nodiscard]] bool IsMainWindow()     const override;
        [[nodiscard]] bool IsVisible()        const override;
        [[nodiscard]] float GetDPIScale()      const override;

        // Called from WndProc, not part of IWindow
        void OnResize(uint32_t aWidth, uint32_t aHeight);
        [[nodiscard]] bool IsMarkedForDestroy() const;
        void MarkForDestroy();

    private:
        void ApplyWindowMode(WindowMode aMode);

        struct Impl;
        std::unique_ptr<Impl> myImpl;

        uint32_t myWidth{ 0 };
        uint32_t myHeight{ 0 };
        bool myIsMainWindow{ false };
        WindowMode myWindowMode{ WindowMode::Windowed };
        
        bool myMarkedForDestroy{ false };
    };
}