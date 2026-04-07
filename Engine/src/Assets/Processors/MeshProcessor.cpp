#include "npch.h"
#include "Nalta/Assets/Processors/MeshProcessor.h"

#include "Nalta/Assets/RawAssetData.h"
#include "Nalta/Assets/Mesh.h"
#include "Nalta/Graphics/GPUResourceManager.h"
#include "Nalta/RHI/Types/RHIDescs.h"

namespace Nalta
{
    bool MeshProcessor::Process(const RawMeshData& aRawData, Mesh& outMesh, Graphics::GPUResourceManager& aGpuResources)
    {
        NL_SCOPE_CORE("MeshProcessor");

        if (!aRawData.IsValid())
        {
            NL_ERROR(GCoreLogger, "invalid raw mesh data for '{}'", aRawData.sourcePath.IsEmpty() ? "cooked" : aRawData.sourcePath.GetPath());
            return false;
        }
        
        // Release old GPU resources before overwriting — deferred so in-flight frames finish
        if (outMesh.vertexBuffer.IsValid())
        {
            aGpuResources.DestroyBuffer(outMesh.vertexBuffer);
        }
        if (outMesh.indexBuffer.IsValid())
        {
            aGpuResources.DestroyBuffer(outMesh.indexBuffer);
        }

        // Convert raw vertices to GPU vertex layout
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

        // Vertex buffer
        {
            RHI::BufferCreationDesc desc{};
            desc.size      = gpuVertices.size() * sizeof(MeshVertex);
            desc.stride    = sizeof(MeshVertex);
            desc.access    = RHI::BufferAccessFlags::GpuOnly;
            desc.viewFlags = RHI::BufferViewFlags::ShaderResource;
            desc.debugName = aRawData.sourcePath.GetStem() + " VB";

            RHI::BufferUploadDesc upload{};
            upload.data = std::as_bytes(std::span{ gpuVertices });

            outMesh.vertexBuffer = aGpuResources.UploadBuffer(desc, upload);
            outMesh.vertexBufferIndex = aGpuResources.GetBuffer(outMesh.vertexBuffer)->GetBindlessIndex();
            
            if (!outMesh.vertexBuffer.IsValid())
            {
                NL_ERROR(GCoreLogger, "failed to upload vertex buffer for '{}'", desc.debugName);
                return false;
            }
        }

        // Index buffer
        {
            RHI::BufferCreationDesc desc{};
            desc.size      = aRawData.indices.size() * sizeof(uint32_t);
            desc.stride    = sizeof(uint32_t);
            desc.access    = RHI::BufferAccessFlags::GpuOnly;
            desc.viewFlags = RHI::BufferViewFlags::ShaderResource;
            desc.debugName = aRawData.sourcePath.GetStem() + " IB";

            RHI::BufferUploadDesc upload{};
            upload.data = std::as_bytes(std::span{ aRawData.indices });

            outMesh.indexBuffer = aGpuResources.UploadBuffer(desc, upload);
            outMesh.indexBufferIndex = aGpuResources.GetBuffer(outMesh.indexBuffer)->GetBindlessIndex();
            
            if (!outMesh.indexBuffer.IsValid())
            {
                NL_ERROR(GCoreLogger, "failed to upload index buffer for '{}'", desc.debugName);
                return false;
            }
        }
        
        // Submeshes
        outMesh.submeshes.clear();
        outMesh.submeshes.reserve(aRawData.submeshes.size());
        for (const auto& raw : aRawData.submeshes)
        {
            outMesh.submeshes.push_back(
            {
                .name          = raw.name,
                .indexOffset   = raw.indexOffset,
                .indexCount    = raw.indexCount,
                .vertexOffset  = raw.vertexOffset,
                .vertexCount   = raw.vertexCount,
                .materialIndex = raw.materialIndex
            });
        }


        outMesh.bounds.min[0] = aRawData.boundsMin[0];
        outMesh.bounds.min[1] = aRawData.boundsMin[1];
        outMesh.bounds.min[2] = aRawData.boundsMin[2];
        outMesh.bounds.max[0] = aRawData.boundsMax[0];
        outMesh.bounds.max[1] = aRawData.boundsMax[1];
        outMesh.bounds.max[2] = aRawData.boundsMax[2];

        NL_INFO(GCoreLogger, "processed mesh '{}' ({} vertices, {} indices, {} submeshes)",
            aRawData.sourcePath.GetPath(),
            aRawData.vertices.size(),
            aRawData.indices.size(),
            aRawData.submeshes.size());

        return true;
    }
}
