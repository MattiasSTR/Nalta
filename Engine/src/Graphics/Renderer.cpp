#include "npch.h"
#include "Nalta/Graphics/Renderer.h"
#include "Nalta/Graphics/GPUResourceManager.h"
#include "Nalta/Graphics/RenderView.h"
#include "Nalta/RHI/RHIDevice.h"
#include "Nalta/RHI/Types/RHIDescs.h"

namespace Nalta::Graphics
{
    Renderer::Renderer(GPUResourceManager* aGpuResourceSystem)
        : myGpuResourceSystem(aGpuResourceSystem)
        , mySceneTranslator(aGpuResourceSystem)
    {}

    Renderer::~Renderer() = default;
    
    void Renderer::Initialize()
    {
        N_CORE_ASSERT(!myIsInitialized, "Renderer already initialized");

        myGraphicsContext = myGpuResourceSystem->GetDevice().CreateGraphicsContext();
        N_CORE_ASSERT(myGraphicsContext != nullptr, "Failed to create graphics context");
        
        {
            RHI::ShaderDesc shaderDesc{};
            shaderDesc.filePath  = Paths::EngineAssetDir() / "Shaders" / "Triangle.hlsl";
            shaderDesc.debugName = "Triangle";
            shaderDesc.stages    =
            {
                { .stage = RHI::ShaderStage::Vertex, .entryPoint = "VSMain" },
                { .stage = RHI::ShaderStage::Pixel,  .entryPoint = "PSMain" }
            };

            RHI::GraphicsPipelineDesc pipelineDesc{};
            pipelineDesc.debugName            = "Triangle";
            pipelineDesc.renderTargetFormats  = { RHI::TextureFormat::RGBA8_UNORM };
            pipelineDesc.depth.depthEnabled   = false;

            auto key = myGpuResourceSystem->CreateGraphicsPipeline(shaderDesc, pipelineDesc);
            N_CORE_ASSERT(key.IsValid(), "Failed to create test pipeline");

            NL_INFO(GCoreLogger, "test pipeline created");
        }

        myIsInitialized = true;
    }
    
    void Renderer::Shutdown()
    {
        N_CORE_ASSERT(myIsInitialized, "Renderer was not initialized");

        myGpuResourceSystem->GetDevice().WaitForIdle();

        myGraphicsContext.reset();

        myIsInitialized = false;
    }

    void Renderer::RenderFrame(const Graphics::RenderFrame& aFrame)
    {
        N_CORE_ASSERT(myIsInitialized, "Renderer is not initialized");
        
        myCurrentFrame = &aFrame;

        BeginFrame();

        for (const SurfaceView& surfaceView : aFrame.surfaces)
        {
            RenderSurface(aFrame.scene, surfaceView);
        }

        EndFrame();
        
        myCurrentFrame = nullptr;
    }

    void Renderer::RenderSurface(const SceneView& aScene, const SurfaceView& aSurfaceView)
    {
        RHI::RenderSurface* surface{ myGpuResourceSystem->GetRenderSurface(aSurfaceView.surface) };
        N_CORE_ASSERT(surface != nullptr, "RenderSurface is null");
        
        constexpr float clearColor[]{ 0.01f, 0.01f, 0.01f, 1.0f };

        surface->SetAsRenderTarget(*myGraphicsContext);
        surface->Clear(*myGraphicsContext, clearColor);

        for (const CameraView& camera : aSurfaceView.cameras)
        {
            RenderCamera(aScene, aSurfaceView, camera);
        }

        surface->EndRenderTarget(*myGraphicsContext);
    }

    void Renderer::RenderCamera(const SceneView& aScene, const SurfaceView& aSurfaceView, const CameraView& aCamera)
    {
        //const uint32_t vpX{ static_cast<uint32_t>(aCamera.viewportX * aSurfaceView.width) };
        //const uint32_t vpY{ static_cast<uint32_t>(aCamera.viewportY * aSurfaceView.height) };
        const uint32_t vpWidth{ static_cast<uint32_t>(aCamera.viewportWidth  * aSurfaceView.width) };
        const uint32_t vpHeight{ static_cast<uint32_t>(aCamera.viewportHeight * aSurfaceView.height) };

        myGraphicsContext->SetViewport(vpWidth, vpHeight);
        myGraphicsContext->SetScissor(vpWidth, vpHeight);

        mySceneTranslator.Translate(aScene, aCamera, myRenderView);

        // Draw calls go here
    }
    
    void Renderer::BeginFrame()
    {
        myGpuResourceSystem->GetDevice().BeginFrame();

        for (const SurfaceView& surfaceView : myCurrentFrame->surfaces)
        {
            RHI::RenderSurface* surface{ myGpuResourceSystem->GetRenderSurface(surfaceView.surface) };
            N_CORE_ASSERT(surface != nullptr, "RenderSurface is null");

            if (surface->GetWidth() != surfaceView.width || surface->GetHeight() != surfaceView.height)
            {
                surface->Resize(surfaceView.width, surfaceView.height);
            }
        }

        myGraphicsContext->Reset();
    }

    void Renderer::EndFrame()
    {
        myGraphicsContext->Close();
        myGpuResourceSystem->GetDevice().SubmitContextWork(*myGraphicsContext);
        myGpuResourceSystem->GetDevice().PrePresent();

        for (const SurfaceView& surfaceView : myCurrentFrame->surfaces)
        {
            RHI::RenderSurface* surface{ myGpuResourceSystem->GetRenderSurface(surfaceView.surface) };
            N_CORE_ASSERT(surface != nullptr, "RenderSurface is null");

            surface->Present(true);
        }

        myGpuResourceSystem->GetDevice().EndFrame();
    }
}