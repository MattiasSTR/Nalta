#pragma once
#include "Nalta/Assets/AssetState.h"
#include "Nalta/Core/Math.h"
#include "Nalta/Graphics/GPUResourceKeys.h"

#include <vector>
#include <string>

namespace Nalta
{
    struct MeshVertex
    {
        interop::float3 position;
        interop::float3 normal;
        interop::float4 tangent;
        interop::float2 texCoord;
    };

    struct MeshBounds
    {
        float min[3]{ 0.f, 0.f, 0.f };
        float max[3]{ 0.f, 0.f, 0.f };

        [[nodiscard]] float3 GetCenter() const
        {
            return (float3(min[0], min[1], min[2]) + float3(max[0], max[1], max[2])) * 0.5f;
        }
        [[nodiscard]] float3 GetExtents() const
        {
            return (float3(max[0], max[1], max[2]) - float3(min[0], min[1], min[2])) * 0.5f;
        }
    };

    struct MeshSubmesh
    {
        std::string name;
        uint32_t indexOffset{ 0 };
        uint32_t indexCount{ 0 };
        uint32_t vertexOffset{ 0 };
        uint32_t vertexCount{ 0 };
        int32_t materialIndex{ -1 };
    };

    struct Mesh
    {
        // GPU resources - owned by GpuResourceSystem, keys stored here
        Graphics::BufferKey vertexBuffer{};
        Graphics::BufferKey indexBuffer{};
        
        MeshBounds bounds;
        std::vector<MeshSubmesh> submeshes;
        AssetState state{ AssetState::Unloaded };

        [[nodiscard]] uint32_t GetIndexCount() const
        {
            uint32_t count{ 0 };
            for (const auto& s : submeshes)
            {
                count += s.indexCount;
            }
            return count;
        }

        [[nodiscard]] uint32_t GetVertexCount() const
        {
            uint32_t count{ 0 };
            for (const auto& s : submeshes)
            {
                count += s.vertexCount;
            }
            return count;
        }
    };
}