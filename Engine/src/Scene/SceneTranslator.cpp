#include "npch.h"
#include "Nalta/Scene/SceneTranslator.h"

namespace Nalta
{
    void SceneTranslator::Build(const Scene& aScene, SceneSnapshot& aSnapshot)
    {
        const ComponentRegistry& registry{ aScene.GetRegistry() };
        const TransformSystem& transforms{ aScene.GetTransformSystem() };

        registry.ForEachStaticMesh([&](const StaticMeshData& aMesh)
        {
            StaticMeshDrawEntry& entry{ aSnapshot.meshes.emplace_back() };
            entry.mesh = aMesh.meshKey;
            entry.albedo = aMesh.albedoKey;
            entry.transform = transforms.GetWorldTransform(aMesh.transformKey);
        });
    }
}