#include "npch.h"
#include "Nalta/Platform/win32/Win32Window.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <ShellScalingApi.h>

namespace Nalta
{
    namespace
    {
        constexpr const wchar_t* WINDOW_CLASS{ L"NaltaWindow" };
    }

    struct Win32Window::Impl
    {
        HWND hwnd{ nullptr };
        WINDOWPLACEMENT savedPlacement{};
    };
    
    Win32Window::Win32Window(const WindowDesc& aDesc)
        : myImpl(std::make_unique<Impl>()),
        myWidth(aDesc.width),
        myHeight(aDesc.height),
        myIsMainWindow(aDesc.isMainWindow)
    {
        NL_SCOPE_CORE("Win32Window");
        DWORD style{ WS_OVERLAPPEDWINDOW };
        if (!aDesc.resizable)
        {
            style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        }
        
        RECT rect{ 0, 0, static_cast<LONG>(aDesc.width), static_cast<LONG>(aDesc.height) };
        AdjustWindowRect(&rect, style, FALSE);
        
        const std::wstring wideCaption(aDesc.caption.begin(), aDesc.caption.end());
        
        myImpl->hwnd = CreateWindowExW(
            0,
            WINDOW_CLASS,
            wideCaption.c_str(),
            style,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right  - rect.left,
            rect.bottom - rect.top,
            nullptr, nullptr,
            GetModuleHandleW(nullptr),
            this
        );
        
        if (myImpl->hwnd == nullptr)
        {
            NL_FATAL(GCoreLogger, "CreateWindowExW failed for '{}'", aDesc.caption);
        }
        
        myImpl->savedPlacement.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(myImpl->hwnd, &myImpl->savedPlacement);
        
        if (aDesc.windowMode != WindowMode::Windowed)
        {
            ApplyWindowMode(aDesc.windowMode);
        }
        
        NL_TRACE(GCoreLogger, "created '{}' ({}x{})", aDesc.caption, myWidth, myHeight);
    }
    
    Win32Window::~Win32Window()
    {
        if (myImpl->hwnd != nullptr)
        {
            NL_TRACE(GCoreLogger, "destroying {}x{}", GetWidth(), GetHeight());
            ::DestroyWindow(myImpl->hwnd);
            myImpl->hwnd = nullptr;
        }
    }
    
    void Win32Window::Show()
    {
        ShowWindow(myImpl->hwnd, SW_SHOW);
    }

    void Win32Window::Hide()
    {
        ShowWindow(myImpl->hwnd, SW_HIDE);
    }
    
    void Win32Window::Resize(const uint32_t aWidth, const uint32_t aHeight)
    {
        myWidth  = aWidth;
        myHeight = aHeight;
        SetWindowPos(myImpl->hwnd, nullptr,
            0, 0, static_cast<int>(aWidth), static_cast<int>(aHeight),
            SWP_NOMOVE | SWP_NOZORDER);
    }
    
    void Win32Window::SetWindowMode(const WindowMode aMode)
    {
        if (myWindowMode == aMode) return;
        ApplyWindowMode(aMode);
    }
    
    void Win32Window::ApplyWindowMode(WindowMode aMode)
    {
        NL_SCOPE_CORE("Win32Window");
        if (aMode == WindowMode::Windowed)
        {
            SetWindowLongW(myImpl->hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
            SetWindowPlacement(myImpl->hwnd, &myImpl->savedPlacement);
            SetWindowPos(myImpl->hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
        else
        {
            GetWindowPlacement(myImpl->hwnd, &myImpl->savedPlacement);

            MONITORINFO mi{ sizeof(MONITORINFO) };
            GetMonitorInfoW(MonitorFromWindow(myImpl->hwnd, MONITOR_DEFAULTTONEAREST), &mi);

            const HWND insertAfter{ (aMode == WindowMode::Fullscreen) ? HWND_TOPMOST : HWND_TOP };

            SetWindowLongW(myImpl->hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(myImpl->hwnd, insertAfter,
                mi.rcMonitor.left,
                mi.rcMonitor.top,
                mi.rcMonitor.right  - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_FRAMECHANGED);
        }

        NL_TRACE(GCoreLogger, "window mode changed to {}", static_cast<uint8_t>(aMode));
        myWindowMode = aMode;
    }
    
    void Win32Window::SetTitle(const std::string& aTitle)
    {
        const std::wstring wide(aTitle.begin(), aTitle.end());
        SetWindowTextW(myImpl->hwnd, wide.c_str());
    }
    
    uint32_t Win32Window::GetWidth()  const { return myWidth; }
    uint32_t Win32Window::GetHeight() const { return myHeight; }

    void* Win32Window::GetNativeHandle() const { return myImpl->hwnd; }

    bool Win32Window::IsMainWindow() const { return myIsMainWindow; }
    
    bool Win32Window::IsVisible() const
    {
        return IsWindowVisible(myImpl->hwnd) != 0;
    }

    float Win32Window::GetDPIScale() const
    {
        const UINT dpi = GetDpiForWindow(myImpl->hwnd);
        return static_cast<float>(dpi) / 96.0f;
    }

    void Win32Window::OnResize(const uint32_t aWidth, const uint32_t aHeight)
    {
        myWidth  = aWidth;
        myHeight = aHeight;
    }

    bool Win32Window::IsMarkedForDestroy() const { return myMarkedForDestroy; }

    void Win32Window::MarkForDestroy()
    {
        NL_SCOPE_CORE("Win32Window");
        
        myMarkedForDestroy = true;
        NL_TRACE(GCoreLogger, "marked for destroy");
    }
}
