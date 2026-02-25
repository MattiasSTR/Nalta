#pragma once
#include <cstdint>
#include <string>

namespace Nalta
{
    struct WindowDesc
    {
        uint32_t width{ 1280 };
        uint32_t height{ 720 };
        std::string caption{ "Nalta" };
        bool fullscreen{ false };
        bool resizable{ true };
    };
}