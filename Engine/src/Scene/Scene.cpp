#include "npch.h"
#include "Nalta/Scene/Scene.h"

#include <algorithm>

namespace Nalta
{
    GameObjectKey Scene::CreateObject(const StringID aName, const TransformData& aTransform)
    {
        const TransformKey transformKey{ myTransforms.Allocate() };
        myTransforms.SetLocalTransform(transformKey, aTransform);

        const GameObjectKey key{ myObjects.Insert(GameObject{ GameObjectKey{}, this, transformKey }) };

        // Fix up the key inside the GameObject now that we have it
        myObjects.GetRef(key).myKey = key;

        HierarchyNode node;
        node.name = aName;
        [[maybe_unused]] const GameObjectKey hierarchyKey{ myHierarchy.Insert(std::move(node)) };
        N_CORE_ASSERT(key == hierarchyKey, "Scene: object and hierarchy SlotMaps desynced");

        return key;
    }

    void Scene::DestroyObject(const GameObjectKey aKey)
    {
        myPendingDestroy.push_back(aKey);
    }

    GameObject* Scene::GetObject(const GameObjectKey aKey)
    {
        return myObjects.Get(aKey);
    }

    void Scene::Update()
    {
        FlushPendingDestroys();
        myTransforms.Update();
    }

    void Scene::SetLocalTransform(const GameObjectKey aKey, const TransformData& aTransform)
    {
        myTransforms.SetLocalTransform(myObjects.GetRef(aKey).myTransformKey, aTransform);
    }

    void Scene::SetParent(const GameObjectKey aChild, const GameObjectKey aParent)
    {
        myTransforms.SetParent(myObjects.GetRef(aChild).myTransformKey, myObjects.GetRef(aParent).myTransformKey);

        HierarchyNode& childNode{ myHierarchy.GetRef(aChild) };

        if (childNode.parent.IsValid())
        {
            auto& siblings{ myHierarchy.GetRef(childNode.parent).children };
            std::erase(siblings, aChild);
        }

        childNode.parent = aParent;
        myHierarchy.GetRef(aParent).children.push_back(aChild);
    }

    void Scene::ClearParent(const GameObjectKey aKey)
    {
        HierarchyNode& node{ myHierarchy.GetRef(aKey) };

        if (node.parent.IsValid())
        {
            auto& siblings{ myHierarchy.GetRef(node.parent).children };
            std::erase(siblings, aKey);
            node.parent = GameObjectKey{};
        }

        myTransforms.ClearParent(myObjects.GetRef(aKey).myTransformKey);
    }

    StaticMeshKey Scene::AddStaticMesh(const GameObjectKey aKey, const MeshKey aMeshKey, const TextureKey aAlbedoKey)
    {
        const GameObject& obj{ myObjects.GetRef(aKey) };

        StaticMeshData data;
        data.meshKey = aMeshKey;
        data.albedoKey = aAlbedoKey;
        data.transformKey = obj.myTransformKey;
        data.dirty = true;

        return myRegistry.AddStaticMesh(data);
    }

    void Scene::RemoveStaticMesh(const GameObjectKey aKey)
    {
        GameObject& obj{ myObjects.GetRef(aKey) };
        if (!obj.HasStaticMesh())
        {
            return;
        }

        myRegistry.RemoveStaticMesh(obj.myStaticMeshKey);
        obj.myStaticMeshKey = StaticMeshKey{};
    }

    void Scene::FlushPendingDestroys()
    {
        for (const GameObjectKey key : myPendingDestroy)
        {
            GameObject* obj{ myObjects.Get(key) };
            if (obj == nullptr)
            {
                continue;
            }

            if (obj->HasStaticMesh())
            {
                RemoveStaticMesh(key);
            }

            HierarchyNode& node{ myHierarchy.GetRef(key) };

            if (node.parent.IsValid())
            {
                auto& siblings{ myHierarchy.GetRef(node.parent).children };
                std::erase(siblings, key);
            }

            for (const GameObjectKey childKey : node.children)
            {
                myTransforms.ClearParent(myObjects.GetRef(childKey).myTransformKey);
                myHierarchy.GetRef(childKey).parent = GameObjectKey{};
            }

            myTransforms.Free(obj->myTransformKey);
            myHierarchy.Remove(key);
            myObjects.Remove(key);
        }

        myPendingDestroy.clear();
    }
}
