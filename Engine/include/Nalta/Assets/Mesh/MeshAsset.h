#pragma once
#include "Nalta/Assets/Asset.h"
#include "Nalta/Graphics/Buffers/VertexBufferHandle.h"
#include "Nalta/Graphics/Buffers/IndexBufferHandle.h"

#include <vector>
#include <string>

namespace Nalta
{
    struct MeshVertex
    {
        interop::float3 position;
        interop::float3 normal;
        interop::float4 tangent; // xyz tangent, w = bitangent sign
        interop::float2 texCoord;
    };
    
    struct MeshBounds
    {
        float min[3]{ 0.0f, 0.0f, 0.0f };
        float max[3]{ 0.0f, 0.0f, 0.0f };

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

    class MeshAsset final : public Asset
    {
    public:
        explicit MeshAsset(AssetPath aPath);
        ~MeshAsset() override = default;

        [[nodiscard]] AssetType GetAssetType() const override { return AssetType::Mesh; }
        [[nodiscard]] Graphics::VertexBufferHandle GetVertexBuffer() const { return myVertexBuffer; }
        [[nodiscard]] Graphics::IndexBufferHandle GetIndexBuffer() const { return myIndexBuffer; }
        [[nodiscard]] const MeshBounds& GetBounds() const { return myBounds; }
        [[nodiscard]] const std::vector<MeshSubmesh>& GetSubmeshes() const { return mySubmeshes; }
        [[nodiscard]] uint32_t GetIndexCount() const;
        [[nodiscard]] uint32_t GetVertexCount() const;

    private:
        friend class MeshProcessor;
        friend class AssetManager;

        Graphics::VertexBufferHandle myVertexBuffer;
        Graphics::IndexBufferHandle myIndexBuffer;
        MeshBounds myBounds;
        std::vector<MeshSubmesh> mySubmeshes;
    };
}