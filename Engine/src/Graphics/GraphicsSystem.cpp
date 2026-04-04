#include "npch.h"
#include "Nalta/Graphics/GraphicsSystem.h"

#include "Nalta/Platform/IWindow.h"
#include "Nalta/Platform/WindowHandle.h"

#ifdef N_RHI_D3D12
#include "Nalta/RHI/D3D12/D3D12Device.h"
#elif defined(N_RHI_VULKAN)
#include "Nalta/RHI/Vulkan/VulkanDevice.h"
#endif

namespace Nalta
{
    using namespace Graphics;
    
    static std::unique_ptr<RHI::D3D12::GraphicsContext> context;
    static std::unique_ptr<RHI::D3D12::RenderSurface> surface;
    static std::unique_ptr<RHI::D3D12::PipelineStateObject> pipeline;

    GraphicsSystem::GraphicsSystem() = default;
    GraphicsSystem::~GraphicsSystem() = default;

    void GraphicsSystem::Initialize(const WindowHandle& aHandle)
    {
        NL_SCOPE_CORE("GraphicsSystem");
        //N_CORE_ASSERT(myDevice == nullptr, "already initialized");
        
        RHI::RenderSurfaceDesc rdesc;
        rdesc.height = aHandle->GetHeight();
        rdesc.width = aHandle->GetWidth();
        rdesc.window = aHandle->GetNativeHandle();

        myDevice = std::make_unique<RHI::Device>();
        context = myDevice->CreateGraphicsContext();
        surface = myDevice->CreateRenderSurface(rdesc);
        
        RHI::ShaderDesc shaderDesc{};
        shaderDesc.filePath  = Paths::EngineAssetDir() / "Shaders" / "Triangle.hlsl";
        shaderDesc.debugName = "Triangle";
        shaderDesc.stages    =
        {
            { RHI::ShaderStage::Vertex, "VSMain" },
            { RHI::ShaderStage::Pixel, "PSMain" },
        };
        
        auto shader = myDevice->CreateShader(shaderDesc);
        N_CORE_ASSERT(shader != nullptr, "Triangle shader failed to compile");
        
        RHI::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.shader                  = shader.get();
        pipelineDesc.renderTargetFormats     = { RHI::TextureFormat::RGBA8_SRGB };
        pipelineDesc.depth.depthEnabled      = false;
        pipelineDesc.debugName               = "TrianglePipeline";
        
        pipeline = myDevice->CreateGraphicsPipeline(pipelineDesc);
        N_CORE_ASSERT(pipeline != nullptr, "Triangle pipeline failed to create");
        
        NL_INFO(GCoreLogger, "initialized");
    }

    void GraphicsSystem::Shutdown()
    {
        NL_SCOPE_CORE("GraphicsSystem");
        
        myDevice->WaitForIdle();
        myDevice->DestroyRenderSurface(std::move(surface));
        myDevice->DestroyContext(std::move(context));
        myDevice->DestroyPipeline(std::move(pipeline));
        myDevice.reset();
        
        NL_INFO(GCoreLogger, "shutdown");
    }

    void GraphicsSystem::Test()
    {
        myDevice->BeginFrame();
        context->Reset();
        constexpr float clearColor[]{ 0.01f, 0.01f, 0.01f, 1.0f };
        
        surface->SetAsRenderTarget(*context);
        surface->Clear(*context, clearColor);
        
        context->SetPipeline(*pipeline);
        context->SetPrimitiveTopology(RHI::PrimitiveTopology::TriangleList);
        context->Draw(3);
        
        surface->EndRenderTarget(*context);
        context->Close();
        myDevice->SubmitContextWork(*context);
        
        myDevice->PrePresent();
        surface->Present(true);
        myDevice->EndFrame();
    }
}
