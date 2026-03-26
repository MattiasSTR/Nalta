#pragma once
#include <memory>
#include <vector>

namespace Nalta
{
    class IInputDevice;
    class IKeyboardDevice;
    class IMouseDevice;

    class InputSystem
    {
    public:
        InputSystem();
        ~InputSystem();

        void Initialize();
        void Shutdown();

        // Called by platform system each frame after PollEvents
        void PrepareFrame() const;
        
        [[nodiscard]] IKeyboardDevice* GetKeyboard(uint32_t aIndex = 0) const;
        [[nodiscard]] IMouseDevice* GetMouse(uint32_t aIndex = 0) const;

        // Internal — called by platform to register devices
        void RegisterDevice(std::unique_ptr<IInputDevice> aDevice);
        void UnregisterDevice(const IInputDevice* aDevice);

    private:
        std::vector<std::unique_ptr<IInputDevice>> myDevices;
    };
}