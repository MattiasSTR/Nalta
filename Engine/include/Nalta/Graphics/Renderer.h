#pragma once
#include "RenderFrame.h"
#include "RenderView.h"
#include "Nalta/Graphics/SceneTranslator.h"
#include "Nalta/RHI/RHIContexts.h"

#include <memory>

namespace Nalta
{
    class AssetManager;
}

namespace Nalta::Graphics
{
    class GPUResourceManager;

    class Renderer
    {
    public:
        explicit Renderer(GPUResourceManager* aGpuResourceSystem, AssetManager* aAssetManager);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        void Initialize();
        void Shutdown();

        void RenderFrame(const Graphics::RenderFrame& aFrame);

    private:
        void BeginFrame() const;
        void EndFrame() const;
        void RenderSurface(const SceneView& aScene, const SurfaceView& aSurfaceView);
        void RenderCamera(const SceneView& aScene, const SurfaceView& aSurfaceView, const CameraDesc& aCamera);

        GPUResourceManager* myGpuResourceSystem{ nullptr };
        AssetManager* myAssetManager{ nullptr };
        SceneTranslator mySceneTranslator;
        std::unique_ptr<RHI::GraphicsContext> myGraphicsContext;

        RenderView myRenderView;
        const Graphics::RenderFrame* myCurrentFrame{ nullptr };
        
        std::unordered_map<RenderSurfaceKey, TextureKey> myDepthTextures;
        BufferKey myCameraConstantBuffer{};
        PipelineKey myMeshPipelineKey{};

        bool myIsInitialized{ false };
    };
}
