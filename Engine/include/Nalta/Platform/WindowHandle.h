#pragma once

namespace Nalta
{
    class IWindow;

    class WindowHandle
    {
    public:
        WindowHandle() = default;
        explicit WindowHandle(IWindow* aWindow) : myWindow(aWindow) {}

        [[nodiscard]] bool IsValid() const { return myWindow != nullptr; }
        [[nodiscard]] IWindow* Get() const { return myWindow; }

        IWindow* operator->() const { return myWindow; }
        explicit operator bool() const { return IsValid(); }

        bool operator==(const WindowHandle& aOther) const { return myWindow == aOther.myWindow; }
        bool operator!=(const WindowHandle& aOther) const { return !(*this == aOther); }

    private:
        IWindow* myWindow{ nullptr };
    };
}