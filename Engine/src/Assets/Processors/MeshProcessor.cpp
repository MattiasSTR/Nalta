#include "npch.h"
#include "Nalta/Assets/Processors/MeshProcessor.h"
#include "Nalta/Assets/Processors/RawAssetData.h"
#include "Nalta/Assets/Mesh/MeshAsset.h"
#include "Nalta/Graphics/GraphicsSystem.h"
#include "Nalta/Graphics/Buffers/VertexBufferDesc.h"
#include "Nalta/Graphics/Buffers/IndexBufferDesc.h"

namespace Nalta
{
    MeshProcessor::MeshProcessor(GraphicsSystem* aGraphicsSystem)
        : myGraphicsSystem(aGraphicsSystem)
    {}

    bool MeshProcessor::Process(const RawMeshData& aRawData, MeshAsset& aOutMesh) const
    {
        NL_SCOPE_CORE("MeshProcessor");
        N_CORE_ASSERT(myGraphicsSystem, "MeshProcessor: null graphics system");

        if (!aRawData.IsValid())
        {
            NL_ERROR(GCoreLogger, "invalid raw mesh data for '{}'", aRawData.sourcePath.GetPath());
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
                .texCoord = float2(v.texCoord[0], v.texCoord[1])
            });
        }

        Graphics::VertexBufferDesc vbDesc;
        vbDesc.stride = sizeof(MeshVertex);
        vbDesc.count  = static_cast<uint32_t>(gpuVertices.size());

        aOutMesh.myVertexBuffer = myGraphicsSystem->CreateVertexBuffer(vbDesc, std::as_bytes(std::span(gpuVertices)));

        if (!aOutMesh.myVertexBuffer.IsValid())
        {
            NL_ERROR(GCoreLogger, "failed to create vertex buffer for '{}'", aRawData.sourcePath.GetPath());
            return false;
        }

        // Create index buffer
        Graphics::IndexBufferDesc ibDesc;
        ibDesc.count  = static_cast<uint32_t>(aRawData.indices.size());
        ibDesc.format = Graphics::IndexFormat::Uint32;

        aOutMesh.myIndexBuffer = myGraphicsSystem->CreateIndexBuffer(ibDesc, std::as_bytes(std::span(aRawData.indices)));

        if (!aOutMesh.myIndexBuffer.IsValid())
        {
            NL_ERROR(GCoreLogger, "failed to create index buffer for '{}'", aRawData.sourcePath.GetPath());
            return false;
        }

        // Copy submeshes
        aOutMesh.mySubmeshes.reserve(aRawData.submeshes.size());
        for (const auto& rawSubmesh : aRawData.submeshes)
        {
            aOutMesh.mySubmeshes.push_back(
            {
                .name          = rawSubmesh.name,
                .indexOffset   = rawSubmesh.indexOffset,
                .indexCount    = rawSubmesh.indexCount,
                .vertexOffset  = rawSubmesh.vertexOffset,
                .vertexCount   = rawSubmesh.vertexCount,
                .materialIndex = rawSubmesh.materialIndex
            });
        }

        // Compute bounds
        aOutMesh.myBounds.min = float3(aRawData.boundsMin[0], aRawData.boundsMin[1], aRawData.boundsMin[2]);
        aOutMesh.myBounds.max = float3(aRawData.boundsMax[0], aRawData.boundsMax[1], aRawData.boundsMax[2]);

        NL_INFO(GCoreLogger, "processed mesh '{}' ({} vertices, {} indices, {} submeshes)",
            aRawData.sourcePath.GetPath(),
            aRawData.vertices.size(),
            aRawData.indices.size(),
            aRawData.submeshes.size());

        return true;
    }
}