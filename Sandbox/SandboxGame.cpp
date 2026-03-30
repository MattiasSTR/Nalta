#include "SandboxGame.h"

#include <Nalta/Assets/AssetManager.h>
#include <Nalta/Core/Assert.h>
#include <Nalta/Core/InitContext.h>
#include <Nalta/Core/Math.h>
#include <Nalta/Core/Paths.h>
#include <Nalta/Core/SceneView.h>
#include <Nalta/Core/SceneViewContext.h>
#include <Nalta/Core/UpdateContext.h>
#include <Nalta/Input/PlayerInput.h>

using namespace Nalta;

struct TransformData
{
    interop::float4x4 model;
    interop::float4x4 viewProjection;
};

void SandboxGame::Initialize(const InitContext& aContext)
{
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

void SandboxGame::BuildSceneView(SceneViewContext& aContext)
{
    const float aspectRatio{ static_cast<float>(aContext.width) / static_cast<float>(aContext.height) };
    const float3 forward
    {
        sin(myYaw) * cos(myPitch),
       -sin(myPitch),
        cos(myYaw) * cos(myPitch)
    };
    const float3 target{ myPosition + forward };

    const float4x4 view{ float4x4::look_at(myPosition, target, float3(0, 1, 0)) };
    const frustum f{ frustum::field_of_view_y(Deg2Rad(75.f), aspectRatio, 0.1f, 1000.f) };
    const projection proj{ f, zclip::zero, zdirection::reverse, zplane::finite };
    const float4x4 viewProj{ mul(view, float4x4::perspective(proj)) };

    aContext.view->camera.view = view;
    aContext.view->camera.projection = float4x4::perspective(proj);
    aContext.view->camera.viewProjection = viewProj;
    aContext.view->camera.position = myPosition;
    
    MeshDrawEntry entry;
    entry.mesh = myMeshHandle;
    entry.pipeline = myPipelineHandle;
    entry.albedo = myTextureHandle;
    entry.transform = float4x4::rotation_y(myTime);

    aContext.view->meshes.push_back(entry);
}