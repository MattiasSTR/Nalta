#pragma once
#include "IInputDevice.h"
#include "MouseButton.h"

namespace Nalta
{
    class IMouseDevice : public IInputDevice
    {
    public:
        [[nodiscard]] InputDeviceType GetType() const override { return InputDeviceType::Mouse; }

        [[nodiscard]] virtual bool IsButtonDown(MouseButton aButton) const = 0;
        [[nodiscard]] virtual bool IsButtonPressed(MouseButton aButton) const = 0;
        [[nodiscard]] virtual bool IsButtonReleased(MouseButton aButton) const = 0;

        [[nodiscard]] virtual float GetX() const = 0; // screen position
        [[nodiscard]] virtual float GetY() const = 0;
        [[nodiscard]] virtual float GetDeltaX() const = 0; // frame delta
        [[nodiscard]] virtual float GetDeltaY() const = 0;
        [[nodiscard]] virtual float GetScroll() const = 0;
    };
}