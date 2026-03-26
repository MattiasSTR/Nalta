#pragma once
#include <cstdint>

namespace Nalta
{
    enum class Key : uint16_t
    {
        // Unknown
        Unknown = 0,

        // Alphabet
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        // Numbers
        Num0, Num1, Num2, Num3, Num4,
        Num5, Num6, Num7, Num8, Num9,

        // Function keys
        F1,  F2,  F3,  F4,  F5,  F6,
        F7,  F8,  F9,  F10, F11, F12,

        // Navigation
        Up, Down, Left, Right,
        PageUp, PageDown,
        Home, End,
        Insert, Delete,

        // Modifiers
        LeftShift, RightShift,
        LeftCtrl,  RightCtrl,
        LeftAlt,   RightAlt,
        LeftSuper, RightSuper,

        // Special
        Space, Enter, Escape, Backspace, Tab,
        CapsLock, NumLock, ScrollLock,
        PrintScreen, Pause,

        // Numpad
        Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
        Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
        NumpadAdd, NumpadSub, NumpadMul, NumpadDiv,
        NumpadDecimal, NumpadEnter,

        // Punctuation
        Comma, Period, Slash, Backslash,
        Semicolon, Apostrophe, Grave,
        LeftBracket, RightBracket, Minus, Equal,

        Count
    };
}