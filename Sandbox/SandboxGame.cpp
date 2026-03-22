#include "SandboxGame.h"

#include <array>
#include <Nalta/Core/Assert.h>
#include <Nalta/Core/Paths.h>
#include <Nalta/Core/InitContext.h>
#include <Nalta/Core/UpdateContext.h>
#include <Nalta/Core/RenderFrameContext.h>
#include <Nalta/Graphics/GraphicsSystem.h>
#include <Nalta/Graphics/ShaderDesc.h>
#include <Nalta/Graphics/PipelineDesc.h>
#include <Nalta/Graphics/RenderFrame.h>
#include <Nalta/Core/Math.h>
#include <Nalta/Graphics/ShaderCompiler.h>

void SandboxGame::Initialize(const Nalta::InitContext& aContext)
{
    Nalta::Graphics::ShaderDesc shaderDesc;
    shaderDesc.filePath = Nalta::Paths::EngineAssetDir() / "Shaders" / "Triangle.hlsl";
    shaderDesc.stages   =
    {
            { Nalta::Graphics::ShaderStage::Vertex, "VSMain" },
            { Nalta::Graphics::ShaderStage::Pixel,  "PSMain" }
    };

    auto shader{ aContext.graphicsSystem->GetShaderCompiler()->Compile(shaderDesc) };
    N_ASSERT(shader, "SandboxGame: failed to compile triangle shader");

    Nalta::Graphics::PipelineDesc pipelineDesc;
    pipelineDesc.shader = shader;

    myTrianglePipeline = aContext.graphicsSystem->CreatePipeline(pipelineDesc);
    N_ASSERT(myTrianglePipeline.IsValid(), "SandboxGame: failed to create triangle pipeline");
    
    struct Vertex
    {
        interop::float3 position;
        interop::float3 color;
    };

    static_assert(sizeof(Vertex) == 24, "Vertex must be 24 bytes");

    const std::array<Vertex, 3> vertices
    {
        Vertex{ float3( 0.0f,  0.5f, 0.0f), float3(1.0f, 0.0f, 0.0f) },
        Vertex{ float3( 0.5f, -0.5f, 0.0f), float3(0.0f, 1.0f, 0.0f) },
        Vertex{ float3(-0.5f, -0.5f, 0.0f), float3(0.0f, 0.0f, 1.0f) }
    };

    Nalta::Graphics::VertexBufferDesc vbDesc;
    vbDesc.stride = sizeof(Vertex);
    vbDesc.count  = static_cast<uint32_t>(vertices.size());

    myTriangleVB = aContext.graphicsSystem->CreateVertexBuffer(vbDesc, std::as_bytes(std::span(vertices)));

    N_ASSERT(myTriangleVB.IsValid(), "SandboxGame: failed to create vertex buffer");
}

void SandboxGame::Shutdown()
{
    myTrianglePipeline = Nalta::Graphics::PipelineHandle{};
}

void SandboxGame::Update(const Nalta::UpdateContext&)
{
    // Nothing yet
}

void SandboxGame::BuildRenderFrame(Nalta::RenderFrameContext& aContext)
{
    aContext.frame.SetPipeline(myTrianglePipeline);
    aContext.frame.SetVertexBuffer(myTriangleVB);
    aContext.frame.Draw(3);
}