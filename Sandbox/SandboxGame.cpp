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
    // Shader
    Nalta::Graphics::ShaderDesc shaderDesc;
    shaderDesc.filePath = Nalta::Paths::EngineAssetDir() / "Shaders" / "Cube.hlsl";
    shaderDesc.stages =
    {
        { Nalta::Graphics::ShaderStage::Vertex, "VSMain" },
        { Nalta::Graphics::ShaderStage::Pixel,  "PSMain" }
    };

    auto shader{ aContext.graphicsSystem->GetShaderCompiler()->Compile(shaderDesc) };
    N_ASSERT(shader, "SandboxGame: failed to compile cube shader");

    Nalta::Graphics::PipelineDesc pipelineDesc;
    pipelineDesc.shader = shader;
    //pipelineDesc.rasterizer.cullMode = Nalta::Graphics::CullMode::None;
    pipelineDesc.depth.depthEnabled = true;
    pipelineDesc.depth.depthWrite = true;

    myCubePipeline = aContext.graphicsSystem->CreatePipeline(pipelineDesc);
    N_ASSERT(myCubePipeline.IsValid(), "SandboxGame: failed to create cube pipeline");

    // Cube vertices — 8 corners
    struct Vertex
    {
        interop::float3 position;
        interop::float3 color;
    };

    static_assert(sizeof(Vertex) == 24, "Vertex must be 24 bytes");

    const std::array<Vertex, 8> vertices
    {
        Vertex{ float3{-0.5f,  0.5f,  0.5f}, float3{1.0f, 0.0f, 0.0f} }, // 0 top    left  front  red
        Vertex{ float3{ 0.5f,  0.5f,  0.5f}, float3{0.0f, 1.0f, 0.0f} }, // 1 top    right front  green
        Vertex{ float3{ 0.5f, -0.5f,  0.5f}, float3{0.0f, 0.0f, 1.0f} }, // 2 bottom right front  blue
        Vertex{ float3{-0.5f, -0.5f,  0.5f}, float3{1.0f, 1.0f, 0.0f} }, // 3 bottom left  front  yellow
        Vertex{ float3{-0.5f,  0.5f, -0.5f}, float3{1.0f, 0.0f, 1.0f} }, // 4 top    left  back   magenta
        Vertex{ float3{ 0.5f,  0.5f, -0.5f}, float3{0.0f, 1.0f, 1.0f} }, // 5 top    right back   cyan
        Vertex{ float3{ 0.5f, -0.5f, -0.5f}, float3{1.0f, 0.5f, 0.0f} }, // 6 bottom right back   orange
        Vertex{ float3{-0.5f, -0.5f, -0.5f}, float3{0.5f, 0.0f, 1.0f} }, // 7 bottom left  back   purple
    };

    // 6 faces x 2 triangles x 3 indices = 36 indices
    constexpr std::array<uint32_t, 36> indices
    {
        // Front
        0, 2, 1,  0, 3, 2,
        // Back
        5, 7, 4,  5, 6, 7,
        // Left
        4, 3, 0,  4, 7, 3,
        // Right
        1, 6, 5,  1, 2, 6,
        // Top
        4, 1, 5,  4, 0, 1,
        // Bottom
        3, 6, 2,  3, 7, 6
    };

    Nalta::Graphics::VertexBufferDesc vbDesc;
    vbDesc.stride = sizeof(Vertex);
    vbDesc.count  = static_cast<uint32_t>(vertices.size());
    myCubeVB = aContext.graphicsSystem->CreateVertexBuffer(vbDesc, std::as_bytes(std::span(vertices)));
    N_ASSERT(myCubeVB.IsValid(), "SandboxGame: failed to create cube vertex buffer");

    Nalta::Graphics::IndexBufferDesc ibDesc;
    ibDesc.count  = static_cast<uint32_t>(indices.size());
    ibDesc.format = Nalta::Graphics::IndexFormat::Uint32;
    myCubeIB = aContext.graphicsSystem->CreateIndexBuffer(ibDesc, std::as_bytes(std::span(indices)));
    N_ASSERT(myCubeIB.IsValid(), "SandboxGame: failed to create cube index buffer");

    // Transform constant buffer — model + viewProjection
    struct TransformData
    {
        interop::float4x4 model;
        interop::float4x4 viewProjection;
    };

    Nalta::Graphics::ConstantBufferDesc cbDesc;
    cbDesc.size = sizeof(TransformData);
    myTransformCB = aContext.graphicsSystem->CreateConstantBuffer(cbDesc);
    N_ASSERT(myTransformCB.IsValid(), "SandboxGame: failed to create transform buffer");
}

void SandboxGame::Shutdown()
{
    myCubePipeline = Nalta::Graphics::PipelineHandle{};
    myCubeVB       = Nalta::Graphics::VertexBufferHandle{};
    myCubeIB       = Nalta::Graphics::IndexBufferHandle{};
    myTransformCB  = Nalta::Graphics::ConstantBufferHandle{};
}

void SandboxGame::Update(const Nalta::UpdateContext& aContext)
{
    myTime += aContext.deltaTime;

    const auto* input{ aContext.playerInput };
    const float speed{ 5.0f * aContext.deltaTime };
    constexpr float sensitivity{ 0.005f };

    // Mouse look
    if (input->IsMouseButtonDown(Nalta::MouseButton::Right))
    {
        myYaw   += input->GetMouseDeltaX() * sensitivity;
        myPitch += input->GetMouseDeltaY() * sensitivity;
        myPitch  = clamp(myPitch, -Nalta::HALF_PI + 0.01f, Nalta::HALF_PI - 0.01f);
    }
    
    // Build forward and right vectors from yaw
    const float3 forward
    {
        sin(myYaw) * cos(myPitch),
       -sin(myPitch),
        cos(myYaw) * cos(myPitch)
    };
    const float3 right  { cos(myYaw), 0.0f, -sin(myYaw) };

    if (input->IsKeyDown(Nalta::Key::W)) myPosition = myPosition + forward * speed;
    if (input->IsKeyDown(Nalta::Key::S)) myPosition = myPosition - forward * speed;
    if (input->IsKeyDown(Nalta::Key::A)) myPosition = myPosition - right   * speed;
    if (input->IsKeyDown(Nalta::Key::D)) myPosition = myPosition + right   * speed;
    if (input->IsKeyDown(Nalta::Key::E)) myPosition.y += speed;
    if (input->IsKeyDown(Nalta::Key::Q)) myPosition.y -= speed;
}

void SandboxGame::BuildRenderFrame(Nalta::RenderFrameContext& aContext)
{
    const float aspectRatio{ static_cast<float>(aContext.width) / static_cast<float>(aContext.height) };
    const float4x4 model{ float4x4::rotation_y(myTime) };
    const float3 target{ myPosition + float3(
        sin(myYaw) * cos(myPitch),
       -sin(myPitch),
        cos(myYaw) * cos(myPitch)) };
    
    const float4x4 view{ float4x4::look_at(myPosition, target, float3(0.0f, 1.0f, 0.0f)) };
    
    const frustum f{ frustum::field_of_view_y(
        Nalta::Deg2Rad(75.0f),
        aspectRatio,
        0.1f,
        1000.0f) };

    const projection proj{ f, zclip::zero, zdirection::reverse, zplane::finite };
    const float4x4 viewProj{ mul(view, float4x4::perspective(proj)) };
    
    struct TransformData
    {
        interop::float4x4 model;
        interop::float4x4 viewProjection;
    };

    const TransformData data{ model, viewProj };

    aContext.frame.UpdateConstantBuffer(myTransformCB, &data, sizeof(data));
    aContext.frame.SetPipeline(myCubePipeline);
    aContext.frame.SetConstantBuffer(myTransformCB, 0);
    aContext.frame.SetVertexBuffer(myCubeVB);
    aContext.frame.SetIndexBuffer(myCubeIB);
    aContext.frame.DrawIndexed(myCubeIB->GetIndexCount());
}