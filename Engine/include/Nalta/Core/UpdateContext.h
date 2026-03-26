#pragma once

namespace Nalta
{
    class PlayerInput;
    
    struct UpdateContext
    {
        float deltaTime{ 0.0f };
        PlayerInput* playerInput{ nullptr }; // local player 0
    };
}