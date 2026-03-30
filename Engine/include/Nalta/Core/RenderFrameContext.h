#pragma once

namespace Nalta
{
    struct RenderFrame;
    class AssetManager;

    struct RenderFrameContext
    {
        RenderFrame& frame;
        AssetManager* assetManager;
        uint32_t width { 0 };
        uint32_t height{ 0 };
    };
}