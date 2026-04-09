#pragma once
#include <Nalta/Assets/AssetKeys.h>
#include <Nalta/Core/IGame.h>
#include <Nalta/Core/Math.h>
#include <Nalta/Scene/Scene.h>

class SandboxGame final : public Nalta::IGame
{
public:
    void Initialize(const Nalta::InitContext& aContext) override;
    void Shutdown()                                     override;
    void Update(const Nalta::UpdateContext& aContext)   override;
    void BuildSceneView(Nalta::SceneSnapshot& aSnapshot) override;

private:
    Nalta::Scene myScene;
    Nalta::GameObjectKey myMeshObjectKey;
    Nalta::GameObjectKey myChildMeshObjectKey;
    
    float3 myPosition{ 0.0f, 0.0f, -3.0f };
    float1 myYaw{ 0.0f };
    float1 myPitch{ 0.0f };
    float myTime{ 0.0f };
};
