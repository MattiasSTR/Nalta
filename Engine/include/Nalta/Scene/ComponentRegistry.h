#pragma once
#include "Nalta/Scene/ComponentData.h"
#include "Nalta/Util/SlotMap.h"

namespace Nalta
{
    class ComponentRegistry
    {
    public:
        // Static Mesh
        StaticMeshKey AddStaticMesh(const StaticMeshData& aData);
        void RemoveStaticMesh(StaticMeshKey aKey);
        StaticMeshData* GetStaticMesh(StaticMeshKey aKey);
        const StaticMeshData* GetStaticMesh(StaticMeshKey aKey) const;
        
        template<typename TFunc>
        void ForEachStaticMesh(TFunc&& aFunc)
        {
            myStaticMeshes.ForEach(std::forward<TFunc>(aFunc));
        }

        template<typename TFunc>
        void ForEachStaticMesh(TFunc&& aFunc) const
        {
            myStaticMeshes.ForEach(std::forward<TFunc>(aFunc));
        }
        
    private:
        SlotMap<StaticMeshKey, StaticMeshData> myStaticMeshes;
    };
}