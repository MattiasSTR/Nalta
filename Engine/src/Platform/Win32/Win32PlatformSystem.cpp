#include "npch.h"
#include "Nalta/Platform/win32/Win32PlatformSystem.h"
#include "Nalta/Platform/win32/Win32Window.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace Nalta
{
    namespace
    {
        constexpr const wchar_t* WINDOW_CLASS{ L"NaltaWindow" };
    }
    
    struct Win32PlatformSystem::Impl
    {
        OnWindowDestroyedCallback onWindowDestroyed;
        std::vector<Win32Window*> pendingDestroy;
        
        static LRESULT CALLBACK WndProc(HWND aHwnd, const UINT aMsg, const WPARAM aWparam, const LPARAM aLparam)
        {
            Win32Window* window{ nullptr };

            if (aMsg == WM_CREATE)
            {
                const auto* cs{ reinterpret_cast<CREATESTRUCTW*>(aLparam) };
                window = static_cast<Win32Window*>(cs->lpCreateParams);
                SetWindowLongPtrW(aHwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
            }
            else
            {
                window = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(aHwnd, GWLP_USERDATA));
            }
            
            if (window != nullptr)
            {
                switch (aMsg)
                {
                    case WM_SIZE:
                    {
                        window->OnResize(LOWORD(aLparam), HIWORD(aLparam));
                        return 0;
                    }

                    case WM_CLOSE:
                    {
                        if (window->IsMainWindow())
                        {
                            PostQuitMessage(0);
                        }
                        else
                        {
                            window->MarkForDestroy();
                        }
                        return 0;
                    }
                        
                    default: break;
                }
            }

            return DefWindowProcW(aHwnd, aMsg, aWparam, aLparam);
        }
    };

    Win32PlatformSystem::Win32PlatformSystem() : myImpl(std::make_unique<Impl>()) {}
    Win32PlatformSystem::~Win32PlatformSystem() = default;

    void Win32PlatformSystem::Initialize()
    {
        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = Impl::WndProc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = WINDOW_CLASS;

        if (RegisterClassExW(&wc) == 0u)
        {
            NL_FATAL(GCoreLogger, "Win32PlatformSystem: failed to register window class");
        }
        
        NL_TRACE(GCoreLogger, "Win32PlatformSystem: initialized");
    }

    void Win32PlatformSystem::Shutdown()
    {
        myWindows.clear();
        UnregisterClassW(WINDOW_CLASS, GetModuleHandleW(nullptr));
        NL_TRACE(GCoreLogger, "Win32PlatformSystem: shutdown");
    }

    bool Win32PlatformSystem::PollEvents()
    {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) != 0)
        {
            if (msg.message == WM_QUIT)
            {
                NL_TRACE(GCoreLogger, "Win32PlatformSystem: WM_QUIT received");
                return false;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        
        std::erase_if(myWindows, [&](const std::unique_ptr<Win32Window>& aWindow)
        {
            if (aWindow->IsMarkedForDestroy())
            {
                if (myImpl->onWindowDestroyed)
                {
                    myImpl->onWindowDestroyed(WindowHandle{ aWindow.get() });
                    NL_TRACE(GCoreLogger, "Win32PlatformSystem: window destroyed callback fired");
                }
                return true;
            }
            return false;
        });

        return true;
    }

    WindowHandle Win32PlatformSystem::CreatePlatformWindow(const WindowDesc& aDesc)
    {
        auto window{ std::make_unique<Win32Window>(aDesc) };
        const WindowHandle handle{ window.get() };
        myWindows.push_back(std::move(window));
        NL_TRACE(GCoreLogger, "Win32PlatformSystem: created window '{}'", aDesc.caption);
        return handle;
    }

    void Win32PlatformSystem::DestroyWindow(const WindowHandle aHandle)
    {
        std::erase_if(myWindows, [&](const std::unique_ptr<Win32Window>& w)
        {
            return w.get() == aHandle.Get();
        });
        NL_TRACE(GCoreLogger, "Win32PlatformSystem: destroyed window");
    }

    WindowHandle Win32PlatformSystem::GetMainWindow() const
    {
        const auto it{ std::ranges::find_if(myWindows, [](const std::unique_ptr<Win32Window>& aWindow)
        {
            return aWindow->IsMainWindow();
        }) };

        return it != myWindows.end() ? WindowHandle{ it->get() } : WindowHandle{};
    }

    void Win32PlatformSystem::SetOnWindowDestroyedCallback(OnWindowDestroyedCallback aCallback)
    {
        myImpl->onWindowDestroyed = std::move(aCallback);
    }
}
