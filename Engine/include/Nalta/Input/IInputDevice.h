#pragma once
#include <cstdint>

namespace Nalta
{
    enum class InputDeviceType : uint8_t
    {
        Keyboard,
        Mouse,
        Gamepad
    };

    class IInputDevice
    {
    public:
        virtual ~IInputDevice() = default;

        [[nodiscard]] virtual InputDeviceType GetType()  const = 0;
        [[nodiscard]] virtual uint32_t GetIndex() const = 0;
        [[nodiscard]] virtual bool IsConnected() const = 0;

        // Called by platform system each frame before game sees input
        virtual void PrepareFrame() = 0;
    };
}