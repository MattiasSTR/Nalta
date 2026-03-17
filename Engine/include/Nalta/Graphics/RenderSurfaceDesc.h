#pragma once
#include "Nalta/Platform/WindowHandle.h"
#include <cstdint>

namespace Nalta::Graphics
{
    struct RenderSurfaceDesc
    {
        WindowHandle window;
        uint32_t     bufferCount{ 2 };
    };

}
