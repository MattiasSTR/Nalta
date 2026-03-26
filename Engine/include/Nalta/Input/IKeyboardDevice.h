#pragma once
#include "IInputDevice.h"
#include "Key.h"

namespace Nalta
{
    class IKeyboardDevice : public IInputDevice
    {
    public:
        [[nodiscard]] InputDeviceType GetType() const override { return InputDeviceType::Keyboard; }

        [[nodiscard]] virtual bool IsKeyDown(Key aKey) const = 0; // held
        [[nodiscard]] virtual bool IsKeyPressed(Key aKey) const = 0; // first frame down
        [[nodiscard]] virtual bool IsKeyReleased(Key aKey) const = 0; // first frame up
    };
}