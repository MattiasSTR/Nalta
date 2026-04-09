#include "npch.h"
#include "Nalta/Scene/TransformSystem.h"

namespace Nalta
{
    TransformKey TransformSystem::Allocate()
    {
        const TransformKey key{ myNodes.Insert({}) };
        [[maybe_unused]] const TransformKey worldKey{ myWorldTransforms.Insert(float4x4::identity()) };
        N_CORE_ASSERT(key == worldKey, "TransformSystem: node and world transform SlotMaps desynced");
        return key;
    }

    void TransformSystem::Free(const TransformKey aKey)
    {
        TransformNode* node{ myNodes.Get(aKey) };
        if (node != nullptr && node->parent.IsValid())
        {
            TransformNode* oldParent{ myNodes.Get(node->parent) };
            if (oldParent != nullptr)
            {
                std::erase(oldParent->children, aKey);
            }
        }

        myNodes.Remove(aKey);
        myWorldTransforms.Remove(aKey);
    }

    void TransformSystem::SetLocalTransform(const TransformKey aKey, const TransformData& aTransform)
    {
        TransformNode* node{ myNodes.Get(aKey) };
        if (node == nullptr)
        {
            return;
        }

        node->local = aTransform;
        node->dirty = true;

        MarkChildrenDirty(aKey);
        AddDirtyRoot(aKey);
    }

    void TransformSystem::SetParent(const TransformKey aKey, const TransformKey aParent)
    {
        TransformNode* node{ myNodes.Get(aKey) };
        if (node == nullptr)
        {
            return;
        }

        if (node->parent.IsValid())
        {
            TransformNode* oldParent{ myNodes.Get(node->parent) };
            if (oldParent != nullptr)
            {
                std::erase(oldParent->children, aKey);
            }
        }

        node->parent = aParent;
        node->dirty = true;

        TransformNode* parentNode{ myNodes.Get(aParent) };
        if (parentNode != nullptr)
        {
            parentNode->children.push_back(aKey);
        }

        MarkChildrenDirty(aKey);
        
        // Register the child itself as a dirty root — not its new parent's root.
        // Resolve() will recurse up through the (potentially clean) new parent
        // to get the correct world transform before resolving the child.
        for (const TransformKey existing : myDirtyRoots)
        {
            if (existing == aKey)
            {
                return;
            }
        }
        myDirtyRoots.push_back(aKey);
    }

    void TransformSystem::ClearParent(const TransformKey aKey)
    {
        TransformNode* node{ myNodes.Get(aKey) };
        if (node == nullptr)
        {
            return;
        }

        if (node->parent.IsValid())
        {
            TransformNode* oldParent{ myNodes.Get(node->parent) };
            if (oldParent != nullptr)
            {
                std::erase(oldParent->children, aKey);
            }
        }

        node->parent = TransformKey{};
        node->dirty = true;
        
        for (const TransformKey existing : myDirtyRoots)
        {
            if (existing == aKey)
            {
                return;
            }
        }
        myDirtyRoots.push_back(aKey);
    }

    const TransformData& TransformSystem::GetLocalTransform(const TransformKey aKey) const
    {
        return myNodes.GetRef(aKey).local;
    }

    const float4x4& TransformSystem::GetWorldTransform(const TransformKey aKey) const
    {
        return myWorldTransforms.GetRef(aKey);
    }

    void TransformSystem::Update()
    {
        for (const TransformKey key : myDirtyRoots)
        {
            Resolve(key);
        }
        myDirtyRoots.clear();
    }

    void TransformSystem::MarkChildrenDirty(const TransformKey aKey)
    {
        TransformNode* node{ myNodes.Get(aKey) };
        if (node == nullptr)
        {
            return;
        }

        for (const TransformKey childKey : node->children)
        {
            TransformNode* child{ myNodes.Get(childKey) };
            if (child != nullptr && !child->dirty)
            {
                child->dirty = true;
                MarkChildrenDirty(childKey);
            }
        }
    }

    void TransformSystem::Resolve(const TransformKey aKey)
    {
        TransformNode* node{ myNodes.Get(aKey) };
        if (node == nullptr || !node->dirty)
        {
            return;
        }

        if (node->parent.IsValid())
        {
            Resolve(node->parent);
        }

        const TransformData& local{ node->local };
        const float4x4 localMatrix{ mul(mul(float4x4::scale(local.scale), float4x4(local.rotation)), float4x4::translation(local.position)) };

        float4x4& world{ myWorldTransforms.GetRef(aKey) };

        if (node->parent.IsValid())
        {
            world = mul(localMatrix, myWorldTransforms.GetRef(node->parent));
        }
        else
        {
            world = localMatrix;
        }

        node->dirty = false;

        for (const TransformKey childKey : node->children)
        {
            TransformNode* child{ myNodes.Get(childKey) };
            if (child != nullptr && child->dirty)
                Resolve(childKey);
        }
    }

    void TransformSystem::AddDirtyRoot(TransformKey aKey)
    {
        // Walk up to the root of this node's chain and register it.
        // If any ancestor is already dirty its root is already registered.
        TransformKey current{ aKey };
        while (true)
        {
            const TransformNode* node{ myNodes.Get(current) };
            if (node == nullptr)
            {
                return;
            }

            if (!node->parent.IsValid())
            {
                // Avoid duplicates
                for (const TransformKey existing : myDirtyRoots)
                {
                    if (existing == current)
                    {
                        return;
                    }
                }

                myDirtyRoots.push_back(current);
                return;
            }

            current = node->parent;
        }
    }
}
