#include "npch.h"
#include "Nalta/Platform/win32/Win32PlatformSystem.h"
#include "Nalta/Platform/win32/Win32Window.h"
#include "Nalta/Input/InputSystem.h"
#include "Nalta/Platform/Win32/Win32KeyboardDevice.h"
#include "Nalta/Platform/Win32/Win32MouseDevice.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>

namespace Nalta
{
    namespace
    {
        struct Win32WindowContext
        {
            Win32Window* window{ nullptr };
            Win32PlatformSystem::Impl* impl{ nullptr };
        };
        
        constexpr const wchar_t* WINDOW_CLASS{ L"NaltaWindow" };
        
        Key Win32VKToEKey(const WPARAM aVK)
        {
            if (aVK >= 'A' && aVK <= 'Z')
            {
                return static_cast<Key>(static_cast<uint32_t>(Key::A) + (aVK - 'A'));
            }
            if (aVK >= '0' && aVK <= '9')
            {
                return static_cast<Key>(static_cast<uint32_t>(Key::Num0) + (aVK - '0'));
            }
            if (aVK >= VK_F1 && aVK <= VK_F12)
            {
                return static_cast<Key>(static_cast<uint32_t>(Key::F1) + (aVK - VK_F1));
            }
            if (aVK >= VK_NUMPAD0 && aVK <= VK_NUMPAD9)
            {
                return static_cast<Key>(static_cast<uint32_t>(Key::Numpad0) + (aVK - VK_NUMPAD0));
            }
        
            switch (aVK)
            {
                case VK_SPACE:      return Key::Space;
                case VK_RETURN:     return Key::Enter;
                case VK_ESCAPE:     return Key::Escape;
                case VK_BACK:       return Key::Backspace;
                case VK_TAB:        return Key::Tab;
                case VK_CAPITAL:    return Key::CapsLock;
                case VK_SHIFT:      return Key::LeftShift;
                case VK_LSHIFT:     return Key::LeftShift;
                case VK_RSHIFT:     return Key::RightShift;
                case VK_CONTROL:    return Key::LeftCtrl;
                case VK_LCONTROL:   return Key::LeftCtrl;
                case VK_RCONTROL:   return Key::RightCtrl;
                case VK_MENU:       return Key::LeftAlt;
                case VK_LMENU:      return Key::LeftAlt;
                case VK_RMENU:      return Key::RightAlt;
                case VK_LWIN:       return Key::LeftSuper;
                case VK_RWIN:       return Key::RightSuper;
                case VK_UP:         return Key::Up;
                case VK_DOWN:       return Key::Down;
                case VK_LEFT:       return Key::Left;
                case VK_RIGHT:      return Key::Right;
                case VK_PRIOR:      return Key::PageUp;
                case VK_NEXT:       return Key::PageDown;
                case VK_HOME:       return Key::Home;
                case VK_END:        return Key::End;
                case VK_INSERT:     return Key::Insert;
                case VK_DELETE:     return Key::Delete;
                case VK_NUMLOCK:    return Key::NumLock;
                case VK_SCROLL:     return Key::ScrollLock;
                case VK_SNAPSHOT:   return Key::PrintScreen;
                case VK_PAUSE:      return Key::Pause;
                case VK_ADD:        return Key::NumpadAdd;
                case VK_SUBTRACT:   return Key::NumpadSub;
                case VK_MULTIPLY:   return Key::NumpadMul;
                case VK_DIVIDE:     return Key::NumpadDiv;
                case VK_DECIMAL:    return Key::NumpadDecimal;
                case VK_OEM_COMMA:  return Key::Comma;
                case VK_OEM_PERIOD: return Key::Period;
                case VK_OEM_2:      return Key::Slash;
                case VK_OEM_5:      return Key::Backslash;
                case VK_OEM_1:      return Key::Semicolon;
                case VK_OEM_7:      return Key::Apostrophe;
                case VK_OEM_3:      return Key::Grave;
                case VK_OEM_4:      return Key::LeftBracket;
                case VK_OEM_6:      return Key::RightBracket;
                case VK_OEM_MINUS:  return Key::Minus;
                case VK_OEM_PLUS:   return Key::Equal;
                default:            return Key::Unknown;
            }
        }
    }
    
    struct Win32PlatformSystem::Impl
    {
        OnWindowDestroyedCallback onWindowDestroyed;
        std::vector<Win32Window*> pendingDestroy;
        std::vector<std::unique_ptr<Win32WindowContext>> windowContexts;
        
        InputSystem inputSystem;
        Win32KeyboardDevice* keyboard{ nullptr };
        Win32MouseDevice* mouse{ nullptr };
        
        static LRESULT CALLBACK WndProc(HWND aHwnd, const UINT aMsg, const WPARAM aWparam, const LPARAM aLparam)
        {
            Win32WindowContext* ctx{ nullptr };

            if (aMsg == WM_CREATE)
            {
                const auto* cs{ reinterpret_cast<CREATESTRUCTW*>(aLparam) };
                ctx = static_cast<Win32WindowContext*>(cs->lpCreateParams);
                SetWindowLongPtrW(aHwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
            }
            else
            {
                ctx = reinterpret_cast<Win32WindowContext*>(GetWindowLongPtrW(aHwnd, GWLP_USERDATA));
            }
            
            if (ctx != nullptr)
            {
                Win32Window* window{ ctx->window };
                Impl* impl{ ctx->impl };
                
                switch (aMsg)
                {
                    case WM_SIZE:
                    {
                        window->OnResize(LOWORD(aLparam), HIWORD(aLparam));
                        return 0;
                    }

                    case WM_CLOSE:
                    {
                        window->IsMainWindow() ? PostQuitMessage(0) : window->MarkForDestroy();
                        return 0;
                    }
                        
                    case WM_KEYDOWN:
                    case WM_SYSKEYDOWN:
                    {
                        if (impl->keyboard != nullptr)
                        {
                            impl->keyboard->OnKeyDown(Win32VKToEKey(aWparam));
                        }
                        break;
                    }

                    case WM_KEYUP:
                    case WM_SYSKEYUP:
                    {
                        if (impl->keyboard != nullptr)
                        {
                            impl->keyboard->OnKeyUp(Win32VKToEKey(aWparam));
                        }
                        break;
                    }

                    case WM_LBUTTONDOWN: if (impl->mouse != nullptr) impl->mouse->OnButtonDown(MouseButton::Left);   break;
                    case WM_LBUTTONUP:   if (impl->mouse != nullptr) impl->mouse->OnButtonUp  (MouseButton::Left);   break;
                    case WM_RBUTTONDOWN: if (impl->mouse != nullptr) impl->mouse->OnButtonDown(MouseButton::Right);  break;
                    case WM_RBUTTONUP:   if (impl->mouse != nullptr) impl->mouse->OnButtonUp  (MouseButton::Right);  break;
                    case WM_MBUTTONDOWN: if (impl->mouse != nullptr) impl->mouse->OnButtonDown(MouseButton::Middle); break;
                    case WM_MBUTTONUP:   if (impl->mouse != nullptr) impl->mouse->OnButtonUp  (MouseButton::Middle); break;

                    case WM_MOUSEMOVE:
                    {
                        if (impl->mouse != nullptr)
                        {
                            impl->mouse->OnMouseMove(static_cast<float>(GET_X_LPARAM(aLparam)), static_cast<float>(GET_Y_LPARAM(aLparam)));
                        }
                        break;
                    }

                    case WM_MOUSEWHEEL:
                    {
                        if (impl->mouse != nullptr)
                        {
                            impl->mouse->OnScroll(static_cast<float>(GET_WHEEL_DELTA_WPARAM(aWparam)) / WHEEL_DELTA);
                        }
                        break;
                    }
                        
                    case WM_INPUT:
                    {
                        if (impl->mouse != nullptr)
                        {
                            UINT size{ 0 };
                            GetRawInputData(reinterpret_cast<HRAWINPUT>(aLparam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));

                            std::vector<BYTE> buffer(size);
                            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(aLparam), RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) == size)
                            {
                                const auto* raw{ reinterpret_cast<RAWINPUT*>(buffer.data()) };
                                if (raw->header.dwType == RIM_TYPEMOUSE)
                                {
                                    impl->mouse->OnRawDelta(static_cast<float>(raw->data.mouse.lLastX), static_cast<float>(raw->data.mouse.lLastY));
                                }
                            }
                        }
                        break;
                    }
                        
                    default: break;
                }
            }

            return DefWindowProcW(aHwnd, aMsg, aWparam, aLparam);
        }
    };

    Win32PlatformSystem::Win32PlatformSystem() 
        : myImpl(std::make_unique<Impl>()) {}
    
    Win32PlatformSystem::~Win32PlatformSystem() = default;

    void Win32PlatformSystem::Initialize()
    {
        NL_SCOPE_CORE("Win32PlatformSystem");
        
        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = Impl::WndProc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = WINDOW_CLASS;

        if (RegisterClassExW(&wc) == 0u)
        {
            NL_FATAL(GCoreLogger, "failed to register window class");
        }
        
        myImpl->inputSystem.Initialize();
        
        auto keyboard{ std::make_unique<Win32KeyboardDevice>() };
        myImpl->keyboard = keyboard.get();
        myImpl->inputSystem.RegisterDevice(std::move(keyboard));

        auto mouse{ std::make_unique<Win32MouseDevice>() };
        myImpl->mouse = mouse.get();
        myImpl->inputSystem.RegisterDevice(std::move(mouse));
        
        RAWINPUTDEVICE rid{};
        rid.usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
        rid.usUsage     = 0x02; // HID_USAGE_GENERIC_MOUSE
        rid.dwFlags     = 0;    // RIDEV_NOLEGACY would disable WM_MOUSEMOVE
        rid.hwndTarget  = nullptr;

        if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
        {
            NL_WARN(GCoreLogger, "Win32PlatformSystem: failed to register raw input device");
        }
        
        NL_TRACE(GCoreLogger, "initialized");
    }

    void Win32PlatformSystem::Shutdown()
    {
        NL_SCOPE_CORE("Win32PlatformSystem");
        
        myWindows.clear();
        UnregisterClassW(WINDOW_CLASS, GetModuleHandleW(nullptr));
        
        myImpl->inputSystem.Shutdown();
        
        NL_TRACE(GCoreLogger, "shutdown");
    }

    bool Win32PlatformSystem::PollEvents()
    {
        NL_SCOPE_CORE("Win32PlatformSystem");
        
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) != 0)
        {
            if (msg.message == WM_QUIT)
            {
                NL_TRACE(GCoreLogger, "WM_QUIT received");
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
                    NL_TRACE(GCoreLogger, "window destroyed callback fired");
                }
                return true;
            }
            return false;
        });

        return true;
    }

    void Win32PlatformSystem::TickInput()
    {
        myImpl->inputSystem.PrepareFrame();
    }

    WindowHandle Win32PlatformSystem::CreatePlatformWindow(const WindowDesc& aDesc)
    {
        NL_SCOPE_CORE("Win32PlatformSystem");

        auto ctx{ std::make_unique<Win32WindowContext>() };
        ctx->impl = myImpl.get();

        auto window{ std::make_unique<Win32Window>(aDesc, ctx.get()) };
        ctx->window = window.get();

        const WindowHandle handle{ window.get() };
        myImpl->windowContexts.push_back(std::move(ctx));
        myWindows.push_back(std::move(window));

        NL_TRACE(GCoreLogger, "created window '{}'", aDesc.caption);
        return handle;
    }

    void Win32PlatformSystem::DestroyWindow(const WindowHandle aHandle)
    {
        std::erase_if(myWindows, [&](const std::unique_ptr<Win32Window>& w)
        {
            return w.get() == aHandle.Get();
        });
        NL_TRACE(GCoreLogger, "destroyed window");
    }

    WindowHandle Win32PlatformSystem::GetMainWindow() const
    {
        const auto it{ std::ranges::find_if(myWindows, [](const std::unique_ptr<Win32Window>& aWindow)
        {
            return aWindow->IsMainWindow();
        }) };

        return it != myWindows.end() ? WindowHandle{ it->get() } : WindowHandle{};
    }

    InputSystem& Win32PlatformSystem::GetInputSystem()
    {
        return myImpl->inputSystem;
    }

    void Win32PlatformSystem::SetOnWindowDestroyedCallback(OnWindowDestroyedCallback aCallback)
    {
        myImpl->onWindowDestroyed = std::move(aCallback);
    }

    void Win32PlatformSystem::SetCurrentThreadName(const std::string& aName) const
    {
        const int size{ MultiByteToWideChar(CP_UTF8, 0, aName.c_str(), -1, nullptr, 0) };
        std::wstring wide(size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, aName.c_str(), -1, wide.data(), size);
        SetThreadDescription(GetCurrentThread(), wide.c_str());
    }

    uint32_t Win32PlatformSystem::GetCPUCoreCount() const
    {
        SYSTEM_INFO si{};
        GetSystemInfo(&si);
        return static_cast<uint32_t>(si.dwNumberOfProcessors);
    }

    uint64_t Win32PlatformSystem::GetSystemMemoryBytes() const
    {
        MEMORYSTATUSEX ms{};
        ms.dwLength = sizeof(ms);
        GlobalMemoryStatusEx(&ms);
        return static_cast<uint64_t>(ms.ullTotalPhys);
    }
}
