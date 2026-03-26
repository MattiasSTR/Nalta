#pragma once
#include "Nalta/Input/IMouseDevice.h"

#include <bitset>

namespace Nalta
{
    class Win32MouseDevice final : public IMouseDevice
    {
    public:
        Win32MouseDevice() = default;
        ~Win32MouseDevice() override = default;

        [[nodiscard]] uint32_t GetIndex() const override { return 0; }
        [[nodiscard]] bool IsConnected() const override { return true; }

        void PrepareFrame() override;

        [[nodiscard]] bool IsButtonDown(MouseButton aButton) const override;
        [[nodiscard]] bool IsButtonPressed(MouseButton aButton) const override;
        [[nodiscard]] bool IsButtonReleased(MouseButton aButton) const override;
        [[nodiscard]] float GetX() const override;
        [[nodiscard]] float GetY() const override;
        [[nodiscard]] float GetDeltaX() const override;
        [[nodiscard]] float GetDeltaY() const override;
        [[nodiscard]] float GetScroll() const override;

        // Called by Win32 window proc
        void OnButtonDown(MouseButton aButton);
        void OnButtonUp(MouseButton aButton);
        void OnMouseMove(float aX, float aY);
        void OnRawDelta(float aDeltaX, float aDeltaY);
        void OnScroll(float aDelta);

    private:
        static constexpr uint32_t BUTTON_COUNT{ static_cast<uint32_t>(MouseButton::Count) };

        // Buttons - double buffered
        std::bitset<BUTTON_COUNT> myButtonBack{};
        std::bitset<BUTTON_COUNT> myButtonFront{};
        std::bitset<BUTTON_COUNT> myButtonPrevious{};

        // Position - double buffered
        float myBackX{ 0.0f };
        float myBackY{ 0.0f };
        float myFrontX{ 0.0f };
        float myFrontY{ 0.0f };
        
        // Delta - double buffered
        float myBackRawDeltaX { 0.0f };
        float myBackRawDeltaY { 0.0f };
        float myFrontRawDeltaX{ 0.0f };
        float myFrontRawDeltaY{ 0.0f };

        // Scroll - accumulated on back, reset each PrepareFrame
        float myBackScroll { 0.0f };
        float myFrontScroll{ 0.0f };
    };
}
