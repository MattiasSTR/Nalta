#pragma once
#include "Nalta/Core/SceneView.h"
#include "Nalta/Graphics/Commands/RenderFrame.h"
#include "Nalta/Graphics/Buffers/ConstantBufferHandle.h"

namespace Nalta
{
    class AssetManager;
    class GraphicsSystem;
}

namespace Nalta::Graphics
{
    class SceneRenderer
    {
    public:
        void Initialize(GraphicsSystem* aGraphicsSystem);
        void Shutdown();

        // Reads the scene view, resolves assets, fills the frame with commands.
        void BuildFrame(const AssetManager* aAssetManager, const SceneView& aView, RenderFrame& outFrame) const;

    private:
        static constexpr uint32_t RootParam_Transform{ 0 };
        static constexpr uint32_t RootParam_Texture{ 1 };

        ConstantBufferHandle myTransformCB;
        GraphicsSystem* myGraphicsSystem{ nullptr };
    };
}