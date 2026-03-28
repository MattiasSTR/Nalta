#pragma once

namespace Nalta
{
    struct RenderFrame;

    struct RenderFrameContext
    {
        RenderFrame& frame;
        uint32_t width { 0 };
        uint32_t height{ 0 };
    };
}