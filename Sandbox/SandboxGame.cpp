#include "SandboxGame.h"

#include <array>
#include <Nalta/Core/Assert.h>
#include <Nalta/Core/InitContext.h>
#include <Nalta/Core/Math.h>
#include <Nalta/Core/Paths.h>
#include <Nalta/Core/RenderFrameContext.h>
#include <Nalta/Core/UpdateContext.h>
#include <Nalta/Graphics/GraphicsSystem.h>
#include <Nalta/Graphics/Commands/RenderFrame.h>
#include <Nalta/Graphics/Pipeline/PipelineDesc.h>
#include <Nalta/Graphics/Shader/ShaderCompiler.h>
#include <Nalta/Graphics/Shader/ShaderDesc.h>
#include <Nalta/Input/PlayerInput.h>

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
    
    constexpr std::array<uint32_t, 3> indices{ 0, 1, 2 };

    Nalta::Graphics::IndexBufferDesc ibDesc;
    ibDesc.count  = static_cast<uint32_t>(indices.size());
    ibDesc.format = Nalta::Graphics::IndexFormat::Uint32;

    myTriangleIB = aContext.graphicsSystem->CreateIndexBuffer(
        ibDesc,
        std::as_bytes(std::span(indices)));

    N_ASSERT(myTriangleIB.IsValid(), "SandboxGame: failed to create index buffer");
    
    Nalta::Graphics::ConstantBufferDesc cbDesc;
    cbDesc.size = sizeof(float);
    myFrameDataCB = aContext.graphicsSystem->CreateConstantBuffer(cbDesc);
    N_ASSERT(myFrameDataCB.IsValid(), "SandboxGame: failed to create frame data buffer");
}

void SandboxGame::Shutdown()
{
    myTrianglePipeline = Nalta::Graphics::PipelineHandle{};
}

void SandboxGame::Update(const Nalta::UpdateContext& aContext)
{
    myTime += aContext.deltaTime;

    const auto* input{ aContext.playerInput };

    // Pressed — should fire ONCE on first frame
    if (input->IsKeyPressed(Nalta::Key::Space))
        NL_INFO(Nalta::GGameLogger, "Space PRESSED");
    if (input->IsKeyPressed(Nalta::Key::W))
        NL_INFO(Nalta::GGameLogger, "W PRESSED");

    // Down — should fire EVERY frame while held
    if (input->IsKeyDown(Nalta::Key::W))
        NL_INFO(Nalta::GGameLogger, "W DOWN (dt: {:.4f})", aContext.deltaTime);

    // Released — should fire ONCE on release
    if (input->IsKeyReleased(Nalta::Key::W))
        NL_INFO(Nalta::GGameLogger, "W RELEASED");

    // Mouse position
    if (input->IsMouseButtonPressed(Nalta::MouseButton::Left))
        NL_INFO(Nalta::GGameLogger, "LMB PRESSED at ({:.1f}, {:.1f})",
            input->GetMouseX(), input->GetMouseY());

    if (input->IsMouseButtonDown(Nalta::MouseButton::Left))
        NL_INFO(Nalta::GGameLogger, "LMB DOWN at ({:.1f}, {:.1f})",
            input->GetMouseX(), input->GetMouseY());

    if (input->IsMouseButtonReleased(Nalta::MouseButton::Left))
        NL_INFO(Nalta::GGameLogger, "LMB RELEASED at ({:.1f}, {:.1f})",
            input->GetMouseX(), input->GetMouseY());

    // Mouse delta — only log if moving
    const float dx{ input->GetMouseDeltaX() };
    const float dy{ input->GetMouseDeltaY() };
    if (std::abs(dx) > 0.5f || std::abs(dy) > 0.5f)
        NL_INFO(Nalta::GGameLogger, "Mouse delta ({:.1f}, {:.1f})", dx, dy);

    // Scroll
    const float scroll{ input->GetMouseScroll() };
    if (std::abs(scroll) > 0.0f)
        NL_INFO(Nalta::GGameLogger, "Scroll {:.2f}", scroll);

    // Modifier keys
    if (input->IsKeyPressed(Nalta::Key::LeftShift))
        NL_INFO(Nalta::GGameLogger, "Left Shift PRESSED");
    if (input->IsKeyReleased(Nalta::Key::LeftShift))
        NL_INFO(Nalta::GGameLogger, "Left Shift RELEASED");

    // Escape to verify single press works for quit-like actions
    if (input->IsKeyPressed(Nalta::Key::Escape))
        NL_INFO(Nalta::GGameLogger, "Escape PRESSED - would quit");
}

void SandboxGame::BuildRenderFrame(Nalta::RenderFrameContext& aContext)
{
    aContext.frame.UpdateConstantBuffer(myFrameDataCB, &myTime, sizeof(myTime));
    aContext.frame.SetPipeline(myTrianglePipeline);
    aContext.frame.SetConstantBuffer(myFrameDataCB, 0);
    aContext.frame.SetVertexBuffer(myTriangleVB);
    aContext.frame.SetIndexBuffer(myTriangleIB);
    aContext.frame.DrawIndexed(3);
}