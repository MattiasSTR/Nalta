#include "SandboxGame.h"

#include <Nalta/Assets/AssetManager.h>
#include <Nalta/Assets/Mesh/MeshAsset.h>
#include <Nalta/Assets/Pipeline/PipelineAsset.h>
#include <Nalta/Core/Assert.h>
#include <Nalta/Core/InitContext.h>
#include <Nalta/Core/Math.h>
#include <Nalta/Core/Paths.h>
#include <Nalta/Core/RenderFrameContext.h>
#include <Nalta/Core/UpdateContext.h>
#include <Nalta/Graphics/GraphicsSystem.h>
#include <Nalta/Graphics/Commands/RenderFrame.h>
#include <Nalta/Input/PlayerInput.h>

void SandboxGame::Initialize(const Nalta::InitContext& aContext)
{
    // Transform constant buffer
    struct TransformData
    {
        interop::float4x4 model;
        interop::float4x4 viewProjection;
    };
    
    Nalta::Graphics::ConstantBufferDesc cbDesc;
    cbDesc.size = sizeof(TransformData);
    myTransformCB = aContext.graphicsSystem->CreateConstantBuffer(cbDesc);
    N_ASSERT(myTransformCB.IsValid(), "SandboxGame: failed to create transform buffer");
    
    myPipelineRequest = aContext.assetManager->Request<Nalta::PipelineAsset>(Nalta::AssetPath(Nalta::Paths::EngineAssetDir() / "Pipelines" / "Mesh.pipeline"));
    myMeshRequest = aContext.assetManager->Request<Nalta::MeshAsset>(Nalta::AssetPath(Nalta::Paths::EngineAssetDir() / "Meshes" / "mesh.obj"));
}

void SandboxGame::Shutdown()
{
    myMeshRequest  = Nalta::AssetRequest{};
    myPipelineRequest  = Nalta::AssetRequest{};
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
    const auto mesh{ myMeshRequest.GetHandle<Nalta::MeshAsset>().Get() };
    if (!mesh || !mesh->IsReady())
    {
        return;
    }
    
    const auto pipeline{ myPipelineRequest.GetHandle<Nalta::PipelineAsset>().Get() };
    if (!pipeline || !pipeline->IsReady())
    {
        return;
    }
    
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
    aContext.frame.SetPipeline(pipeline->GetPipelineHandle());
    aContext.frame.SetConstantBuffer(myTransformCB, 0);
    aContext.frame.SetVertexBuffer(mesh->GetVertexBuffer());
    aContext.frame.SetIndexBuffer(mesh->GetIndexBuffer());
    aContext.frame.DrawIndexed(mesh->GetIndexCount());
}