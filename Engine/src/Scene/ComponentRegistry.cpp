#include "npch.h"
#include "Nalta/Scene/ComponentRegistry.h"

namespace Nalta
{
    StaticMeshKey ComponentRegistry::AddStaticMesh(const StaticMeshData& aData)
    {
        return myStaticMeshes.Insert(aData);
    }

    void ComponentRegistry::RemoveStaticMesh(const StaticMeshKey aKey)
    {
        myStaticMeshes.Remove(aKey);
    }

    StaticMeshData* ComponentRegistry::GetStaticMesh(const StaticMeshKey aKey)
    {
        return myStaticMeshes.Get(aKey);
    }

    const StaticMeshData* ComponentRegistry::GetStaticMesh(const StaticMeshKey aKey) const
    {
        return myStaticMeshes.Get(aKey);
    }
}