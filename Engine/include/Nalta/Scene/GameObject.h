#pragma once
#include "Nalta/Scene/ComponentData.h"

namespace Nalta
{
    class Scene;
    
    // GameObject
    //
    // Thin handle representing an entity in the scene.
    // Holds keys into Scene's packed arrays - no data beyond identity
    // and which capabilities are attached.
    // All mutation routes through Scene - the interception point
    // for undo/redo, serialization, and render proxy delta commands.
    
    class GameObject
    {
    public:
        [[nodiscard]] GameObjectKey GetKey() const { return myKey; }
        [[nodiscard]] bool IsValid() const { return myKey.IsValid(); }
        
        // Transform
        void SetPosition(const float3& aPosition) const;
        void SetRotation(const quaternion& aRotation) const;
        void SetScale(const float3& aScale) const;
        void SetLocalTransform(const TransformData& aTransform) const;

        [[nodiscard]] const TransformData& GetLocalTransform() const;
        [[nodiscard]] const float4x4& GetWorldTransform() const;
        
        // Hierarchy
        void SetParent(const GameObject& aParent) const;
        void ClearParent() const;
        
        // Static Mesh
        void AddStaticMesh(MeshKey aMeshKey, TextureKey aAlbedoKey);
        void RemoveStaticMesh();
        [[nodiscard]] bool HasStaticMesh() const { return myStaticMeshKey.IsValid(); }
        [[nodiscard]] StaticMeshKey GetStaticMeshKey() const { return myStaticMeshKey; }

        void SetMesh(MeshKey aMeshKey) const;
        void SetAlbedo(TextureKey aAlbedoKey) const;
        
    private:
        friend class Scene;

        GameObject(const GameObjectKey aKey, Scene* aScene, const TransformKey aTransformKey)
            : myKey(aKey)
            , myScene(aScene)
            , myTransformKey(aTransformKey)
        {}

        GameObjectKey myKey;
        Scene* myScene{ nullptr };
        TransformKey myTransformKey;
        
        // Invalid if not attached
        StaticMeshKey myStaticMeshKey;
    };
}