#include "npch.h"
#include "Nalta/Assets/Processors/MeshProcessor.h"
#include "Nalta/Assets/Processors/RawAssetData.h"
#include "Nalta/Assets/Mesh/MeshAsset.h"
#include "Nalta/Graphics/GraphicsSystem.h"
#include "Nalta/Graphics/Buffers/VertexBufferDesc.h"
#include "Nalta/Graphics/Buffers/IndexBufferDesc.h"

namespace Nalta
{
    bool MeshProcessor::Process(const RawAssetData& aBaseData, Asset& aOutAsset, GraphicsSystem& aGraphicsSystem) const
    {
        NL_SCOPE_CORE("MeshProcessor");

        const auto& aRawData{ static_cast<const RawMeshData&>(aBaseData) };
        auto& meshAsset{ static_cast<MeshAsset&>(aOutAsset) };

        if (!aRawData.IsValid())
        {
            NL_ERROR(GCoreLogger, "invalid raw mesh data for '{}'", aRawData.sourcePath.IsEmpty() ? "cooked" : aRawData.sourcePath.GetPath());
            return false;
        }

        std::vector<MeshVertex> gpuVertices;
        gpuVertices.reserve(aRawData.vertices.size());

        for (const auto& v : aRawData.vertices)
        {
            gpuVertices.push_back(
            {
                .position = float3(v.position[0], v.position[1], v.position[2]),
                .normal   = float3(v.normal[0],   v.normal[1],   v.normal[2]),
                .tangent  = float4(v.tangent[0],  v.tangent[1],  v.tangent[2], v.tangent[3]),
                .texCoord = float2(v.texCoord[0], v.texCoord[1])
            });
        }

        Graphics::VertexBufferDesc vbDesc;
        vbDesc.stride = sizeof(MeshVertex);
        vbDesc.count  = static_cast<uint32_t>(gpuVertices.size());

        meshAsset.myVertexBuffer = aGraphicsSystem.CreateVertexBuffer(vbDesc, std::as_bytes(std::span(gpuVertices)));

        if (!meshAsset.myVertexBuffer.IsValid())
        {
            NL_ERROR(GCoreLogger, "failed to create vertex buffer");
            return false;
        }

        Graphics::IndexBufferDesc ibDesc;
        ibDesc.count  = static_cast<uint32_t>(aRawData.indices.size());
        ibDesc.format = Graphics::IndexFormat::Uint32;

        meshAsset.myIndexBuffer = aGraphicsSystem.CreateIndexBuffer(ibDesc, std::as_bytes(std::span(aRawData.indices)));

        if (!meshAsset.myIndexBuffer.IsValid())
        {
            NL_ERROR(GCoreLogger, "failed to create index buffer");
            return false;
        }

        meshAsset.mySubmeshes.reserve(aRawData.submeshes.size());
        for (const auto& rawSubmesh : aRawData.submeshes)
        {
            meshAsset.mySubmeshes.push_back(
            {
                .name          = rawSubmesh.name,
                .indexOffset   = rawSubmesh.indexOffset,
                .indexCount    = rawSubmesh.indexCount,
                .vertexOffset  = rawSubmesh.vertexOffset,
                .vertexCount   = rawSubmesh.vertexCount,
                .materialIndex = rawSubmesh.materialIndex
            });
        }

        meshAsset.myBounds.min[0] = aRawData.boundsMin[0];
        meshAsset.myBounds.min[1] = aRawData.boundsMin[1];
        meshAsset.myBounds.min[2] = aRawData.boundsMin[2];
        meshAsset.myBounds.max[0] = aRawData.boundsMax[0];
        meshAsset.myBounds.max[1] = aRawData.boundsMax[1];
        meshAsset.myBounds.max[2] = aRawData.boundsMax[2];

        NL_INFO(GCoreLogger, "processed mesh '{}' ({} vertices, {} indices, {} submeshes)",
            aRawData.sourcePath.IsEmpty() ? "cooked" : aRawData.sourcePath.GetPath(),
            aRawData.vertices.size(),
            aRawData.indices.size(),
            aRawData.submeshes.size());

        return true;
    }
}