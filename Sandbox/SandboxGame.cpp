#include "SandboxGame.h"

#include <Nalta/Assets/AssetManager.h>
#include <Nalta/Core/Assert.h>
#include <Nalta/Core/InitContext.h>
#include <Nalta/Core/Math.h>
#include <Nalta/Core/Paths.h>
#include <Nalta/Core/SceneSnapshot.h>
#include <Nalta/Core/UpdateContext.h>
#include <Nalta/Input/PlayerInput.h>
#include <Nalta/Scene/SceneTranslator.h>

using namespace Nalta;

void SandboxGame::Initialize([[maybe_unused]] const InitContext& aContext)
{
    const auto meshKey{ aContext.assetManager->RequestMesh(AssetPath(Paths::EngineAssetDir() / "Meshes" / "mesh.obj")) };
    const auto textureKey{ aContext.assetManager->RequestTexture(AssetPath(Paths::EngineAssetDir() / "Textures" / "test.texture")) };

    myMeshObjectKey = myScene.CreateObject(StringID{ "Mesh" });
    myScene.GetObject(myMeshObjectKey)->AddStaticMesh(meshKey, textureKey);
    
    TransformData transform;
    transform.scale = { 0.25f, 0.25f, 0.25f };
    transform.position = { 2.0f, 0.0f, 0.0f };
    myChildMeshObjectKey = myScene.CreateObject(StringID{ "ChildMesh" }, transform);
    myScene.GetObject(myChildMeshObjectKey)->AddStaticMesh({}, textureKey);
    myScene.GetObject(myChildMeshObjectKey)->SetParent(*myScene.GetObject(myMeshObjectKey));
}

void SandboxGame::Shutdown()
{
    
}

void SandboxGame::Update(const UpdateContext& aContext)
{
    myScene.Update();
    
    myTime += aContext.deltaTime;
    if (myTime > TWO_PI)
    {
        myTime -= TWO_PI;
    } 
    
    myScene.GetObject(myMeshObjectKey)->SetRotation(quaternion::rotation_y(myTime));
    myScene.GetObject(myChildMeshObjectKey)->SetPosition({ 2.f, sin(myTime), 0.f });


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

void SandboxGame::BuildSceneView([[maybe_unused]] SceneSnapshot& aSnapshot)
{
    const float3 forward
    {
        sin(myYaw) * cos(myPitch),
       -sin(myPitch),
        cos(myYaw) * cos(myPitch)
    };
    const float3 target{ myPosition + forward };
    
    CameraDesc& camera{ aSnapshot.cameras.emplace_back() };
    camera.view = float4x4::look_at(myPosition, target, float3(0, 1, 0));
    camera.position = myPosition;
    
    SceneTranslator::Build(myScene, aSnapshot);
}