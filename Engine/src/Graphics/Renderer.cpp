#include "npch.h"
#include "Nalta/Graphics/Renderer.h"

#include "Nalta/Assets/AssetManager.h"
#include "Nalta/Graphics/GPUResourceManager.h"
#include "Nalta/Graphics/RenderView.h"
#include "Nalta/RHI/RHIDevice.h"
#include "Nalta/RHI/Types/RHIDescs.h"

// NOTE:
// ALL OF THIS IS TESTING AND WILL BE REPLACED

namespace Nalta::Graphics
{
    namespace
    {
        struct CameraConstants
        {
            float4x4 viewProj;
        };
        
        struct MeshDrawConstants
        {
            uint32_t vertexBufferIndex;
            uint32_t albedoIndex;
            uint32_t pad2;
            uint32_t pad3;
            float4x4 transform;
        };
    }
    
    Renderer::Renderer(GPUResourceManager* aGpuResourceSystem, AssetManager* aAssetManager)
        : myGpuResourceSystem(aGpuResourceSystem)
        , myAssetManager(aAssetManager)
    {}

    Renderer::~Renderer() = default;
    
    void Renderer::Initialize()
    {
        N_CORE_ASSERT(!myIsInitialized, "Renderer already initialized");

        myGraphicsContext = myGpuResourceSystem->GetDevice().CreateGraphicsContext();
        N_CORE_ASSERT(myGraphicsContext != nullptr, "Failed to create graphics context");

        RHI::BufferCreationDesc desc{};
        desc.size      = sizeof(CameraConstants);
        desc.access    = RHI::BufferAccessFlags::CpuToGpu;
        desc.viewFlags = RHI::BufferViewFlags::ConstantBuffer;
        desc.debugName = "CameraConstants";

        myCameraConstantBuffer = myGpuResourceSystem->CreateBuffer(desc);
        
        RHI::ShaderDesc shaderDesc{};
        shaderDesc.filePath  = Paths::EngineAssetDir() / "Shaders" / "Mesh.hlsl";
        shaderDesc.debugName = "Mesh";
        shaderDesc.stages    =
        {
            { .stage = RHI::ShaderStage::Vertex, .entryPoint = "VSMain" },
            { .stage = RHI::ShaderStage::Pixel,  .entryPoint = "PSMain" }
        };

        RHI::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.debugName           = "Mesh";
        pipelineDesc.renderTargetFormats = { RHI::TextureFormat::RGBA8_SRGB };
        pipelineDesc.depthFormat         = RHI::TextureFormat::D32_Float;
        pipelineDesc.depth.depthEnabled  = true;
        pipelineDesc.depth.depthWrite    = true;
        pipelineDesc.depth.compareFunc   = RHI::DepthCompare::Greater;
        pipelineDesc.rasterizer.cullMode = RHI::CullMode::Back;

        myMeshPipelineKey = myGpuResourceSystem->CreateGraphicsPipeline(shaderDesc, pipelineDesc);
        N_CORE_ASSERT(myMeshPipelineKey.IsValid(), "Failed to create mesh pipeline");

        myIsInitialized = true;
    }
    
    void Renderer::Shutdown()
    {
        N_CORE_ASSERT(myIsInitialized, "Renderer was not initialized");

        myGpuResourceSystem->GetDevice().WaitForIdle();

        for (auto& depthKey : myDepthStencils | std::views::values)
        {
            if (depthKey.IsValid())
            {
                myGpuResourceSystem->DestroyTexture(depthKey);
            }
        }
        myDepthStencils.clear();

        myGraphicsContext.reset();
        myIsInitialized = false;
    }

    void Renderer::RenderFrame(const Graphics::RenderFrame& aFrame)
    {
        N_CORE_ASSERT(myIsInitialized, "Renderer is not initialized");
        
        BeginFrame(aFrame);

        for (const SurfaceView& surfaceView : aFrame.surfaces)
        {
            RenderSurface(surfaceView.scene, surfaceView);
        }

        EndFrame(aFrame);
    }

    void Renderer::RenderSurface(const SceneSnapshot& aScene, const SurfaceView& aSurfaceView)
    {
        RHI::RenderSurface* surface{ myGpuResourceSystem->GetRenderSurface(aSurfaceView.surface) };
        N_CORE_ASSERT(surface != nullptr, "RenderSurface is null");

        const auto depthKey{ EnsureDepthStencil(aSurfaceView) };
        const RHI::Texture* depthTexture{ myGpuResourceSystem->GetTexture(depthKey) };

        constexpr float clearColor[]{ 0.01f, 0.01f, 0.01f, 1.0f };
        
        surface->SetAsRenderTarget(*myGraphicsContext, depthTexture);
        surface->Clear(*myGraphicsContext, clearColor);
        myGraphicsContext->ClearDepthStencil(*depthTexture);

        for (const CameraDesc& camera : aScene.cameras)
        {
            RenderCamera(aScene, aSurfaceView, camera);
        }

        surface->EndRenderTarget(*myGraphicsContext);
    }

    void Renderer::RenderCamera(const SceneSnapshot& aScene, const SurfaceView& aSurfaceView, const CameraDesc& aCamera)
    {
        //const uint32_t vpX{ static_cast<uint32_t>(aCamera.viewportX * aSurfaceView.width) };
        //const uint32_t vpY{ static_cast<uint32_t>(aCamera.viewportY * aSurfaceView.height) };
        const uint32_t vpWidth{ static_cast<uint32_t>(aCamera.viewportWidth  * aSurfaceView.width) };
        const uint32_t vpHeight{ static_cast<uint32_t>(aCamera.viewportHeight * aSurfaceView.height) };
        
        const float aspectRatio{ static_cast<float>(aSurfaceView.width) / static_cast<float>(aSurfaceView.height) };
        const frustum f{ frustum::field_of_view_y(Deg2Rad(aCamera.fovY), aspectRatio, aCamera.nearPlane, aCamera.farPlane) };
        const projection proj{ f, zclip::zero, zdirection::reverse, zplane::finite };
        const float4x4 viewProj{ mul(aCamera.view, float4x4::perspective(proj)) };
        
        const RHI::Buffer* cameraCB{ myGpuResourceSystem->GetBuffer(myCameraConstantBuffer) };
        CameraConstants cameraConstants{ .viewProj = viewProj };
        std::memcpy(cameraCB->mappedData, &cameraConstants, sizeof(CameraConstants));
        
        myGraphicsContext->SetViewport(vpWidth, vpHeight);
        myGraphicsContext->SetScissor(vpWidth, vpHeight);
        myGraphicsContext->SetPassCBV(cameraCB->gpuAddress);
        myGraphicsContext->SetPipeline(*myGpuResourceSystem->GetPipeline(myMeshPipelineKey));
        myGraphicsContext->SetPrimitiveTopology(RHI::PrimitiveTopology::TriangleList);

        for (const StaticMeshDrawEntry& entry : aScene.meshes)
        {
            const Mesh* mesh{ myAssetManager->GetMesh(entry.mesh) };
            const Texture* albedo{ myAssetManager->GetTexture(entry.albedo) };

            const RHI::Buffer* ib{ myGpuResourceSystem->GetBuffer(mesh->indexBuffer) };

            MeshDrawConstants meshConstants{};
            meshConstants.vertexBufferIndex = mesh->vertexBufferIndex;
            meshConstants.albedoIndex = albedo->textureIndex;
            meshConstants.transform = entry.transform;
            myGraphicsContext->SetRootConstants(meshConstants);
            
            myGraphicsContext->SetIndexBuffer(*ib);

            for (const MeshSubmesh& submesh : mesh->submeshes)
            {
                myGraphicsContext->DrawIndexed(submesh.indexCount, submesh.indexOffset, submesh.vertexOffset);
            }
        }
    }

    TextureKey Renderer::EnsureDepthStencil(const SurfaceView& aSurfaceView)
    {
        auto& depthKey{ myDepthStencils[aSurfaceView.surface] };
        if (!depthKey.IsValid() || myGpuResourceSystem->GetTexture(depthKey)->width  != aSurfaceView.width ||
            myGpuResourceSystem->GetTexture(depthKey)->height != aSurfaceView.height)
        {
            if (depthKey.IsValid())
            {
                myGpuResourceSystem->DestroyTexture(depthKey);
            }

            RHI::TextureCreationDesc depthDesc{};
            depthDesc.width     = aSurfaceView.width;
            depthDesc.height    = aSurfaceView.height;
            depthDesc.format    = RHI::TextureFormat::D32_Float;
            depthDesc.viewFlags = RHI::TextureViewFlags::DepthStencil;
            depthDesc.debugName = "SceneDepth";

            depthKey = myGpuResourceSystem->CreateTexture(depthDesc);
        }
        
        return depthKey;
    }

    void Renderer::BeginFrame(const Graphics::RenderFrame& aFrame) const
    {
        myGpuResourceSystem->GetDevice().BeginFrame();

        for (const SurfaceView& surfaceView : aFrame.surfaces)
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

    void Renderer::EndFrame(const Graphics::RenderFrame& aFrame) const
    {
        myGraphicsContext->Close();
        myGpuResourceSystem->GetDevice().SubmitContextWork(*myGraphicsContext);
        myGpuResourceSystem->GetDevice().PrePresent();

        for (const SurfaceView& surfaceView : aFrame.surfaces)
        {
            RHI::RenderSurface* surface{ myGpuResourceSystem->GetRenderSurface(surfaceView.surface) };
            N_CORE_ASSERT(surface != nullptr, "RenderSurface is null");

            surface->Present(true);
        }

        myGpuResourceSystem->GetDevice().EndFrame();
    }
}