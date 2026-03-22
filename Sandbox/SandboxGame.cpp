#include "SandboxGame.h"

#include <Nalta/Core/Assert.h>
#include <Nalta/Core/Paths.h>
#include <Nalta/Core/InitContext.h>
#include <Nalta/Core/UpdateContext.h>
#include <Nalta/Core/RenderFrameContext.h>
#include <Nalta/Graphics/GraphicsSystem.h>
#include <Nalta/Graphics/ShaderDesc.h>
#include <Nalta/Graphics/PipelineDesc.h>
#include <Nalta/Graphics/RenderFrame.h>

void SandboxGame::Initialize(const Nalta::InitContext& aContext)
{
    Nalta::Graphics::ShaderDesc shaderDesc;
    shaderDesc.filePath = Nalta::Paths::EngineAssetDir() / "Shaders" / "Triangle.hlsl";
    shaderDesc.stages   =
    {
            { Nalta::Graphics::ShaderStage::Vertex, "VSMain" },
            { Nalta::Graphics::ShaderStage::Pixel,  "PSMain" }
    };

    auto shader{ aContext.graphicsSystem->GetShaderCompiler().Compile(shaderDesc) };
    N_ASSERT(shader, "SandboxGame: failed to compile triangle shader");

    Nalta::Graphics::PipelineDesc pipelineDesc;
    pipelineDesc.shader = shader;

    myTrianglePipeline = aContext.graphicsSystem->CreatePipeline(pipelineDesc);
    N_ASSERT(myTrianglePipeline.IsValid(), "SandboxGame: failed to create triangle pipeline");
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
    aContext.frame.Draw(3);
}