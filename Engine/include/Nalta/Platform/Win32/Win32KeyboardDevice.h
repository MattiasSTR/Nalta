#pragma once
#include "Nalta/Input/IKeyboardDevice.h"

#include <bitset>

namespace Nalta
{
    class Win32KeyboardDevice final : public IKeyboardDevice
    {
    public:
        Win32KeyboardDevice() = default;
        ~Win32KeyboardDevice() override = default;

        [[nodiscard]] uint32_t GetIndex() const override { return 0; }
        [[nodiscard]] bool IsConnected() const override { return true; }

        void PrepareFrame() override;

        [[nodiscard]] bool IsKeyDown(Key aKey) const override;
        [[nodiscard]] bool IsKeyPressed(Key aKey) const override;
        [[nodiscard]] bool IsKeyReleased(Key aKey) const override;

        // Called by Win32 window proc
        void OnKeyDown(Key aKey);
        void OnKeyUp(Key aKey);

    private:
        static constexpr uint32_t KEY_COUNT{ static_cast<uint32_t>(Key::Count) };

        std::bitset<KEY_COUNT> myBack{};    // main thread writes
        std::bitset<KEY_COUNT> myFront{};   // update thread reads
        std::bitset<KEY_COUNT> myPrevious{};
    };
}
