#pragma once
#include "Nalta/Platform/IWindow.h"
#include <string>

struct GLFWwindow;

namespace Nalta
{
    struct WindowDesc;

    class WindowsWindow : public IWindow
    {
    public:
        explicit WindowsWindow(const WindowDesc& aDesc);
        ~WindowsWindow() override;

        [[nodiscard]] bool IsClosed() const override;

        [[nodiscard]] uint32_t GetWidth() const override;
        [[nodiscard]] uint32_t GetHeight() const override;
        void Resize(uint32_t aWidth, uint32_t aHeight) override;

        void SetFullscreen(bool aFullscreen) override;
        [[nodiscard]] bool IsFullscreen() const override;

        void SetCaption(const std::string& aCaption) override;

        [[nodiscard]] void* GetNativeHandle() const override;

    private:
        GLFWwindow* myNativeWindow{ nullptr };
        uint32_t myWidth{ 0 };
        uint32_t myHeight{ 0 };
        bool myFullscreen{ false };
    };
}