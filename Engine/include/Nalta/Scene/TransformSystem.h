#pragma once
#include "Nalta/Core/Math.h"
#include "Nalta/Scene/ComponentData.h"
#include "Nalta/Util/SlotMap.h"

namespace Nalta
{
    // Owns all transform nodes and resolves the parent chain each frame
    // Everything else references transforms via TransformKey
    class TransformSystem
    {
    public:
        TransformKey Allocate();
        void Free(TransformKey aKey);
        
        void SetLocalTransform(TransformKey aKey, const TransformData& aTransform);
        void SetParent(TransformKey aKey, TransformKey aParent);
        void ClearParent(TransformKey aKey);
        
        [[nodiscard]] const TransformData& GetLocalTransform(TransformKey aKey) const;
        [[nodiscard]] const float4x4& GetWorldTransform(TransformKey aKey) const;
        
        // Call once per frame
        // Resolves all dirty nodes in parent-before-child order
        void Update();
        
    private:
        void MarkChildrenDirty(TransformKey aKey);
        void Resolve(TransformKey aKey);
        void AddDirtyRoot(TransformKey aKey);
        
        struct TransformNode
        {
            TransformData local;
            TransformKey parent;   // invalid if root
            std::vector<TransformKey> children;
            bool dirty{ true };
        };
 
        SlotMap<TransformKey, TransformNode> myNodes;
        SlotMap<TransformKey, float4x4> myWorldTransforms;
        std::vector<TransformKey> myDirtyRoots;
    };
}