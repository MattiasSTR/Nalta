#pragma once
#include "Nalta/Core/StringID.h"
#include "Nalta/Scene/ComponentRegistry.h"
#include "Nalta/Scene/GameObject.h"
#include "Nalta/Scene/TransformSystem.h"
#include "Nalta/Util/SlotMap.h"

#include <vector>

namespace Nalta
{
    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        
        GameObjectKey CreateObject(StringID aName = StringID{ "GameObject" }, const TransformData& aTransform = {});
        void DestroyObject(GameObjectKey aKey);
        GameObject* GetObject(GameObjectKey aKey);
        
        void Update();
        
        // Mutation (called by GameObject)
        void SetLocalTransform(GameObjectKey aKey, const TransformData& aTransform);
        void SetParent(GameObjectKey aChild, GameObjectKey aParent);
        void ClearParent(GameObjectKey aKey);
        
        StaticMeshKey AddStaticMesh(GameObjectKey aKey, MeshKey aMeshKey, TextureKey aAlbedoKey);
        void RemoveStaticMesh(GameObjectKey aKey);
        
        [[nodiscard]] ComponentRegistry& GetRegistry() { return myRegistry; }
        [[nodiscard]] const ComponentRegistry& GetRegistry() const { return myRegistry; }
        [[nodiscard]] TransformSystem& GetTransformSystem() { return myTransforms; }
        [[nodiscard]] const TransformSystem& GetTransformSystem() const { return myTransforms; }
        
    private:
        void FlushPendingDestroys();

        struct HierarchyNode
        {
            StringID name;
            GameObjectKey parent;
            std::vector<GameObjectKey> children;
        };

        SlotMap<GameObjectKey, GameObject> myObjects;
        SlotMap<GameObjectKey, HierarchyNode> myHierarchy;

        ComponentRegistry myRegistry;
        TransformSystem myTransforms;

        std::vector<GameObjectKey> myPendingDestroy;
    };
}
