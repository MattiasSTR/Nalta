#pragma once

namespace Nalta
{
    struct SceneView;

    // Passed to IGame::BuildSceneView(). The game fills the view.
    
    struct SceneViewContext
    {
        SceneView* view{ nullptr };
        uint32_t width{ 0 };
        uint32_t height{ 0 };
    };
}