#pragma once

namespace Nalta
{
    class GraphicsSystem;
    class AssetManager;

    struct InitContext
    {
        GraphicsSystem* graphicsSystem{ nullptr };
        AssetManager* assetManager{ nullptr };
    };
}