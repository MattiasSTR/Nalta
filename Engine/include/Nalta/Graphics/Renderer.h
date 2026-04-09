#pragma once
#include "RenderFrame.h"
#include "RenderView.h"
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
        void BeginFrame(const Graphics::RenderFrame& aFrame) const;
        void EndFrame(const Graphics::RenderFrame& aFrame) const;
        void RenderSurface(const SceneSnapshot& aScene, const SurfaceView& aSurfaceView);
        void RenderCamera(const SceneSnapshot& aScene, const SurfaceView& aSurfaceView, const CameraDesc& aCamera);
        TextureKey EnsureDepthStencil(const SurfaceView& aSurfaceView);

        GPUResourceManager* myGpuResourceSystem{ nullptr };
        AssetManager* myAssetManager{ nullptr };
        std::unique_ptr<RHI::GraphicsContext> myGraphicsContext;

        RenderView myRenderView;
        
        std::unordered_map<RenderSurfaceKey, TextureKey> myDepthStencils;
        BufferKey myCameraConstantBuffer{};
        PipelineKey myMeshPipelineKey{};

        bool myIsInitialized{ false };
    };
}
