#pragma once
#include "Key.h"
#include "MouseButton.h"

namespace Nalta
{
    class IKeyboardDevice;
    class IMouseDevice;

    class PlayerInput
    {
    public:
        PlayerInput() = default;

        void AssignKeyboard(IKeyboardDevice* aKeyboard);
        void AssignMouse   (IMouseDevice*    aMouse);

        // Keyboard
        [[nodiscard]] bool IsKeyDown(Key aKey) const;
        [[nodiscard]] bool IsKeyPressed(Key aKey) const;
        [[nodiscard]] bool IsKeyReleased(Key aKey) const;

        // Mouse
        [[nodiscard]] bool IsMouseButtonDown(MouseButton aButton) const;
        [[nodiscard]] bool IsMouseButtonPressed(MouseButton aButton) const;
        [[nodiscard]] bool IsMouseButtonReleased(MouseButton aButton) const;
        [[nodiscard]] float GetMouseX() const;
        [[nodiscard]] float GetMouseY() const;
        [[nodiscard]] float GetMouseDeltaX() const;
        [[nodiscard]] float GetMouseDeltaY() const;
        [[nodiscard]] float GetMouseScroll() const;

    private:
        IKeyboardDevice* myKeyboard{ nullptr };
        IMouseDevice*    myMouse   { nullptr };
    };
}