#include "npch.h"
#include "Nalta/Assets/Mesh/MeshAsset.h"

#include "Nalta/Graphics/Buffers/IIndexBuffer.h"
#include "Nalta/Graphics/Buffers/IVertexBuffer.h"

namespace Nalta
{
    MeshAsset::MeshAsset(AssetPath aPath)
        : Asset(std::move(aPath))
    {}

    uint32_t MeshAsset::GetIndexCount() const
    {
        if (myIndexBuffer.IsValid())
        {
            if (const auto* ib{ myIndexBuffer.Get() })
            {
                return ib->GetIndexCount();
            }
        }
        return 0;
    }

    uint32_t MeshAsset::GetVertexCount() const
    {
        if (myVertexBuffer.IsValid())
        {
            if (const auto* vb{ myVertexBuffer.Get() })
            {
                return vb->GetVertexCount();
            }
        }
        return 0;
    }
}
