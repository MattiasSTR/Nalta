#include "npch.h"
#include "Nalta/Platform/Win32/Win32MouseDevice.h"

namespace Nalta
{
    void Win32MouseDevice::PrepareFrame()
    {
        myButtonPrevious = myButtonFront;
        myButtonFront    = myButtonBack;
        
        myFrontX = myBackX;
        myFrontY = myBackY;

        myFrontScroll = myBackScroll;
        myBackScroll  = 0.0f;
        
        myFrontRawDeltaX = myBackRawDeltaX;
        myFrontRawDeltaY = myBackRawDeltaY;
        myBackRawDeltaX  = 0.0f;
        myBackRawDeltaY  = 0.0f;
    }

    bool Win32MouseDevice::IsButtonDown(const MouseButton aButton) const
    {
        return myButtonFront.test(static_cast<uint32_t>(aButton));
    }

    bool Win32MouseDevice::IsButtonPressed(const MouseButton aButton) const
    {
        const auto index{ static_cast<uint32_t>(aButton) };
        return myButtonFront.test(index) && !myButtonPrevious.test(index);
    }

    bool Win32MouseDevice::IsButtonReleased(const MouseButton aButton) const
    {
        const auto index{ static_cast<uint32_t>(aButton) };
        return !myButtonFront.test(index) && myButtonPrevious.test(index);
    }

    float Win32MouseDevice::GetX() const { return myFrontX; }
    float Win32MouseDevice::GetY() const { return myFrontY; }
    float Win32MouseDevice::GetDeltaX() const { return myFrontRawDeltaX; }
    float Win32MouseDevice::GetDeltaY() const { return myFrontRawDeltaY; }
    float Win32MouseDevice::GetScroll() const { return myFrontScroll; }

    void Win32MouseDevice::OnButtonDown(const MouseButton aButton)
    {
        myButtonBack.set(static_cast<uint32_t>(aButton));
    }

    void Win32MouseDevice::OnButtonUp(const MouseButton aButton)
    {
        myButtonBack.reset(static_cast<uint32_t>(aButton));
    }

    void Win32MouseDevice::OnMouseMove(const float aX, const float aY)
    {
        myBackX = aX;
        myBackY = aY;
    }

    void Win32MouseDevice::OnRawDelta(float aDeltaX, float aDeltaY)
    {
        myBackRawDeltaX += aDeltaX;
        myBackRawDeltaY += aDeltaY;
    }

    void Win32MouseDevice::OnScroll(const float aDelta)
    {
        myBackScroll += aDelta;
    }
}