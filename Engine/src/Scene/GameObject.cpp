#include "npch.h"
#include "Nalta/Scene/GameObject.h"
#include "Nalta/Scene/Scene.h"


namespace Nalta
{
    void GameObject::SetPosition(const float3& aPosition) const
    {
        TransformData t{ GetLocalTransform() };
        t.position = aPosition;
        myScene->SetLocalTransform(myKey, t);
    }

    void GameObject::SetRotation(const quaternion& aRotation) const
    {
        TransformData t{ GetLocalTransform() };
        t.rotation = aRotation;
        myScene->SetLocalTransform(myKey, t);
    }

    void GameObject::SetScale(const float3& aScale) const
    {
        TransformData t{ GetLocalTransform() };
        t.scale = aScale;
        myScene->SetLocalTransform(myKey, t);
    }

    void GameObject::SetLocalTransform(const TransformData& aTransform) const
    {
        myScene->SetLocalTransform(myKey, aTransform);
    }

    const TransformData& GameObject::GetLocalTransform() const
    {
        return myScene->GetTransformSystem().GetLocalTransform(myTransformKey);
    }

    const float4x4& GameObject::GetWorldTransform() const
    {
        return myScene->GetTransformSystem().GetWorldTransform(myTransformKey);
    }

    void GameObject::SetParent(const GameObject& aParent) const
    {
        myScene->SetParent(myKey, aParent.GetKey());
    }

    void GameObject::ClearParent() const
    {
        myScene->ClearParent(myKey);
    }

    void GameObject::AddStaticMesh(const MeshKey aMeshKey, const TextureKey aAlbedoKey)
    {
        N_CORE_ASSERT(!HasStaticMesh(), "GameObject already has a static mesh");
        myStaticMeshKey = myScene->AddStaticMesh(myKey, aMeshKey, aAlbedoKey);
    }

    void GameObject::RemoveStaticMesh()
    {
        if (!HasStaticMesh())
        {
            return;
        }

        myScene->RemoveStaticMesh(myKey);
        myStaticMeshKey = StaticMeshKey{};
    }

    void GameObject::SetMesh(const MeshKey aMeshKey) const
    {
        N_CORE_ASSERT(HasStaticMesh(), "GameObject has no static mesh");
        StaticMeshData* data{ myScene->GetRegistry().GetStaticMesh(myStaticMeshKey) };
        if (data == nullptr)
        {
            return;
        }

        data->meshKey = aMeshKey;
        data->dirty = true;
    }

    void GameObject::SetAlbedo(const TextureKey aAlbedoKey) const
    {
        N_CORE_ASSERT(HasStaticMesh(), "GameObject has no static mesh");
        StaticMeshData* data{ myScene->GetRegistry().GetStaticMesh(myStaticMeshKey) };
        if (data == nullptr)
        {
            return;
        }

        data->albedoKey = aAlbedoKey;
        data->dirty = true;
    }
}
