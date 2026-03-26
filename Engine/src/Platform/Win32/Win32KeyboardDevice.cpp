#include "npch.h"
#include "Nalta/Platform/Win32/Win32KeyboardDevice.h"

namespace Nalta
{
    void Win32KeyboardDevice::PrepareFrame()
    {
        myPrevious = myFront;
        myFront = myBack;
    }

    bool Win32KeyboardDevice::IsKeyDown(const Key aKey) const
    {
        return myFront.test(static_cast<uint32_t>(aKey));
    }

    bool Win32KeyboardDevice::IsKeyPressed(const Key aKey) const
    {
        const auto index{ static_cast<uint32_t>(aKey) };
        return myFront.test(index) && !myPrevious.test(index);
    }

    bool Win32KeyboardDevice::IsKeyReleased(const Key aKey) const
    {
        const auto index{ static_cast<uint32_t>(aKey) };
        return !myFront.test(index) && myPrevious.test(index);
    }

    void Win32KeyboardDevice::OnKeyDown(const Key aKey)
    {
        myBack.set(static_cast<uint32_t>(aKey));
    }

    void Win32KeyboardDevice::OnKeyUp(const Key aKey)
    {
        myBack.reset(static_cast<uint32_t>(aKey));
    }
}