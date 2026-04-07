#include "SandboxGame.h"

#include <Nalta/Assets/AssetManager.h>
#include <Nalta/Core/Assert.h>
#include <Nalta/Core/InitContext.h>
#include <Nalta/Core/Math.h>
#include <Nalta/Core/Paths.h>
#include <Nalta/Core/SceneView.h>
#include <Nalta/Core/UpdateContext.h>
#include <Nalta/Input/PlayerInput.h>

using namespace Nalta;

struct TransformData
{
    interop::float4x4 model;
    interop::float4x4 viewProjection;
};

void SandboxGame::Initialize([[maybe_unused]] const InitContext& aContext)
{
    myMeshKey = aContext.assetManager->RequestMesh(AssetPath(Paths::EngineAssetDir() / "Meshes" / "mesh.obj"));
    myTextureKey = aContext.assetManager->RequestTexture(AssetPath(Paths::EngineAssetDir() / "Textures" / "test.texture"));
}

void SandboxGame::Shutdown()
{
    
}

void SandboxGame::Update(const UpdateContext& aContext)
{
    myTime += aContext.deltaTime;
    if (myTime > TWO_PI)
    {
        myTime -= TWO_PI;
    } 

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

void SandboxGame::BuildSceneView([[maybe_unused]] SceneView& aView)
{
    const float3 forward
    {
        sin(myYaw) * cos(myPitch),
       -sin(myPitch),
        cos(myYaw) * cos(myPitch)
    };
    const float3 target{ myPosition + forward };
    
    CameraDesc& camera{ aView.cameras.emplace_back() };
    camera.view = float4x4::look_at(myPosition, target, float3(0, 1, 0));
    camera.position = myPosition;
    
    MeshDrawEntry& entry{ aView.meshes.emplace_back() };
    entry.mesh = myMeshKey;
    entry.albedo = myTextureKey;
    entry.transform = float4x4::rotation_y(myTime);
}