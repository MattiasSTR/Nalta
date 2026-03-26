#include "npch.h"
#include "Nalta/Input/PlayerInput.h"
#include "Nalta/Input/IKeyboardDevice.h"
#include "Nalta/Input/IMouseDevice.h"

namespace Nalta
{
    void PlayerInput::AssignKeyboard(IKeyboardDevice* aKeyboard)
    {
        myKeyboard = aKeyboard;
    }

    void PlayerInput::AssignMouse(IMouseDevice* aMouse)
    {
        myMouse = aMouse;
    }

    bool PlayerInput::IsKeyDown(const Key aKey) const
    {
        return (myKeyboard != nullptr) && myKeyboard->IsKeyDown(aKey);
    }

    bool PlayerInput::IsKeyPressed(const Key aKey) const
    {
        return (myKeyboard != nullptr) && myKeyboard->IsKeyPressed(aKey);
    }

    bool PlayerInput::IsKeyReleased(const Key aKey) const
    {
        return (myKeyboard != nullptr) && myKeyboard->IsKeyReleased(aKey);
    }

    bool PlayerInput::IsMouseButtonDown(const MouseButton aButton) const
    {
        return (myMouse != nullptr) && myMouse->IsButtonDown(aButton);
    }

    bool PlayerInput::IsMouseButtonPressed(const MouseButton aButton) const
    {
        return (myMouse != nullptr) && myMouse->IsButtonPressed(aButton);
    }

    bool PlayerInput::IsMouseButtonReleased(const MouseButton aButton) const
    {
        return (myMouse != nullptr) && myMouse->IsButtonReleased(aButton);
    }
    
    float PlayerInput::GetMouseX() const
    {
        return (myMouse != nullptr) ? myMouse->GetX() : 0.0f;
    }
    
    float PlayerInput::GetMouseY() const
    {
        return (myMouse != nullptr) ? myMouse->GetY() : 0.0f;
    }
    
    float PlayerInput::GetMouseDeltaX() const
    {
        return (myMouse != nullptr) ? myMouse->GetDeltaX() : 0.0f;
    }
    
    float PlayerInput::GetMouseDeltaY() const
    {
        return (myMouse != nullptr) ? myMouse->GetDeltaY() : 0.0f;
    }
    
    float PlayerInput::GetMouseScroll() const
    {
        return (myMouse != nullptr) ? myMouse->GetScroll() : 0.0f;
    }
}