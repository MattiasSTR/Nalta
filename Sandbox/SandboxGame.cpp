#include "SandboxGame.h"

#include <Nalta/Assets/AssetManager.h>
#include <Nalta/Core/Assert.h>
#include <Nalta/Core/InitContext.h>
#include <Nalta/Core/Math.h>
#include <Nalta/Core/Paths.h>
#include <Nalta/Core/RenderFrameContext.h>
#include <Nalta/Core/UpdateContext.h>
#include <Nalta/Graphics/GraphicsSystem.h>
#include <Nalta/Graphics/Commands/RenderFrame.h>
#include <Nalta/Input/PlayerInput.h>

using namespace Nalta;

struct TransformData
{
    interop::float4x4 model;
    interop::float4x4 viewProjection;
};

void SandboxGame::Initialize(const InitContext& aContext)
{
    Graphics::ConstantBufferDesc cbDesc;
    cbDesc.size = sizeof(TransformData);
    myTransformCB = aContext.graphicsSystem->CreateConstantBuffer(cbDesc);
    N_ASSERT(myTransformCB.IsValid(), "SandboxGame: failed to create transform buffer");
    
    myMeshHandle = aContext.assetManager->RequestMesh(AssetPath(Paths::EngineAssetDir() / "Meshes" / "mesh.obj"));
    myPipelineHandle = aContext.assetManager->RequestPipeline(AssetPath(Paths::EngineAssetDir() / "Pipelines" / "Mesh.pipeline"));
    myTextureHandle = aContext.assetManager->RequestTexture(AssetPath(Paths::EngineAssetDir() / "Textures" / "test.texture"));
}

void SandboxGame::Shutdown()
{
    
}

void SandboxGame::Update(const UpdateContext& aContext)
{
    myTime += aContext.deltaTime;

    const auto* input{ aContext.playerInput };
    const float speed{ 5.0f * aContext.deltaTime };
    constexpr float sensitivity{ 0.005f };

    // Mouse look
    if (input->IsMouseButtonDown(MouseButton::Right))
    {
        myYaw   += input->GetMouseDeltaX() * sensitivity;
        myPitch += input->GetMouseDeltaY() * sensitivity;
        myPitch  = clamp(myPitch, -HALF_PI + 0.01f, HALF_PI - 0.01f);
    }
    
    // Build forward and right vectors from yaw
    const float3 forward
    {
        sin(myYaw) * cos(myPitch),
       -sin(myPitch),
        cos(myYaw) * cos(myPitch)
    };
    const float3 right  { cos(myYaw), 0.0f, -sin(myYaw) };

    if (input->IsKeyDown(Key::W)) myPosition = myPosition + forward * speed;
    if (input->IsKeyDown(Key::S)) myPosition = myPosition - forward * speed;
    if (input->IsKeyDown(Key::A)) myPosition = myPosition - right   * speed;
    if (input->IsKeyDown(Key::D)) myPosition = myPosition + right   * speed;
    if (input->IsKeyDown(Key::E)) myPosition.y += speed;
    if (input->IsKeyDown(Key::Q)) myPosition.y -= speed;
}

void SandboxGame::BuildRenderFrame(RenderFrameContext& aContext)
{
    const auto* mesh{ aContext.assetManager->GetMesh(myMeshHandle) };
    const auto* pipeline{ aContext.assetManager->GetPipeline(myPipelineHandle) };
    const auto* texture{ aContext.assetManager->GetTexture(myTextureHandle) };
    
    // This is still here because I don't have a realiable way yet to determine root parameter indices
    if (!pipeline)
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
        Deg2Rad(75.0f),
        aspectRatio,
        0.1f,
        1000.0f) };

    const projection proj{ f, zclip::zero, zdirection::reverse, zplane::finite };
    const float4x4 viewProj{ mul(view, float4x4::perspective(proj)) };

    const TransformData data{ model, viewProj };

    aContext.frame.UpdateConstantBuffer(myTransformCB, &data, sizeof(data));
    aContext.frame.SetPipeline(pipeline->gpuHandle);
    aContext.frame.SetConstantBuffer(myTransformCB, 0);
    aContext.frame.SetTexture(texture->gpuHandle, 1);
    aContext.frame.SetVertexBuffer(mesh->vb);
    aContext.frame.SetIndexBuffer(mesh->ib);
    aContext.frame.DrawIndexed(mesh->GetIndexCount());
}