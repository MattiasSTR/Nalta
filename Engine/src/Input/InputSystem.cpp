#include "npch.h"
#include "Nalta/Input/InputSystem.h"
#include "Nalta/Input/IInputDevice.h"
#include "Nalta/Input/IKeyboardDevice.h"
#include "Nalta/Input/IMouseDevice.h"

namespace Nalta
{
    InputSystem::InputSystem() = default;
    InputSystem::~InputSystem() = default;

    void InputSystem::Initialize()
    {
        NL_SCOPE_CORE("InputSystem");
        NL_INFO(GCoreLogger, "initialized");
    }

    void InputSystem::Shutdown()
    {
        NL_SCOPE_CORE("InputSystem");
        myDevices.clear();
        NL_INFO(GCoreLogger, "shutdown");
    }

    void InputSystem::PrepareFrame() const
    {
        for (const auto& device : myDevices)
        {
            device->PrepareFrame();
        }
    }
    
    IKeyboardDevice* InputSystem::GetKeyboard(const uint32_t aIndex) const
    {
        uint32_t count{ 0 };
        for (const auto& device : myDevices)
        {
            if (device->GetType() == InputDeviceType::Keyboard)
            {
                if (count == aIndex)
                {
                    return static_cast<IKeyboardDevice*>(device.get());
                }
                ++count;
            }
        }
        return nullptr;
    }

    IMouseDevice* InputSystem::GetMouse(const uint32_t aIndex) const
    {
        uint32_t count{ 0 };
        for (const auto& device : myDevices)
        {
            if (device->GetType() == InputDeviceType::Mouse)
            {
                if (count == aIndex)
                {
                    return static_cast<IMouseDevice*>(device.get());
                }
                ++count;
            }
        }
        return nullptr;
    }
    
    void InputSystem::RegisterDevice(std::unique_ptr<IInputDevice> aDevice)
    {
        NL_SCOPE_CORE("InputSystem");
        
        NL_TRACE(GCoreLogger, "device registered");
        myDevices.push_back(std::move(aDevice));
    }

    void InputSystem::UnregisterDevice(const IInputDevice* aDevice)
    {
        NL_SCOPE_CORE("InputSystem");
        
        std::erase_if(myDevices, [&](const std::unique_ptr<IInputDevice>& d)
        {
            return d.get() == aDevice;
        });
        
        NL_TRACE(GCoreLogger, "device unregistered");
    }
}