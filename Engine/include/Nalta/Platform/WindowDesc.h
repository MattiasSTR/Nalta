#pragma once
#include "IWindow.h"

#include <cstdint>
#include <string>

namespace Nalta
{
    struct WindowDesc
    {
        uint32_t width          { 1280 };
        uint32_t height         { 720 };
        std::string caption     { "Nalta" };
        WindowMode windowMode   { WindowMode::Windowed };
        bool resizable          { true };
        bool isMainWindow       { false };
    };
}