#include "npch.h"
#include "Nalta/Assets/Serializers/MeshSerializer.h"

#include "Nalta/Assets/RawAssetData.h"
#include "Nalta/Core/BinaryIO.h"

namespace Nalta
{
    void MeshSerializer::Write(const RawMeshData& aRawData, BinaryWriter& aWriter)
    {
        NL_SCOPE_CORE("MeshSerializer");

        // Bounds
        aWriter.Write(aRawData.boundsMin[0]);
        aWriter.Write(aRawData.boundsMin[1]);
        aWriter.Write(aRawData.boundsMin[2]);
        aWriter.Write(aRawData.boundsMax[0]);
        aWriter.Write(aRawData.boundsMax[1]);
        aWriter.Write(aRawData.boundsMax[2]);

        // Submeshes
        aWriter.Write(static_cast<uint32_t>(aRawData.submeshes.size()));
        for (const auto& submesh : aRawData.submeshes)
        {
            aWriter.WriteString(submesh.name);
            aWriter.Write(submesh.vertexOffset);
            aWriter.Write(submesh.vertexCount);
            aWriter.Write(submesh.indexOffset);
            aWriter.Write(submesh.indexCount);
            aWriter.Write(submesh.materialIndex);
        }

        // Vertices
        aWriter.Write(static_cast<uint32_t>(aRawData.vertices.size()));
        aWriter.WriteBytes(std::span(reinterpret_cast<const uint8_t*>(aRawData.vertices.data()), aRawData.vertices.size() * sizeof(RawVertex)));

        // Indices
        aWriter.Write(static_cast<uint32_t>(aRawData.indices.size()));
        aWriter.WriteBytes(std::span(reinterpret_cast<const uint8_t*>(aRawData.indices.data()), aRawData.indices.size() * sizeof(uint32_t)));

        // Materials
        aWriter.Write(static_cast<uint32_t>(aRawData.materials.size()));
        for (const auto& mat : aRawData.materials)
        {
            aWriter.WriteString(mat.name);
            aWriter.WriteBytes(std::span(reinterpret_cast<const uint8_t*>(mat.baseColor), sizeof(mat.baseColor)));
            aWriter.Write(mat.roughness);
            aWriter.Write(mat.metallic);
            aWriter.WriteString(mat.albedoPath);
            aWriter.WriteString(mat.normalPath);
            aWriter.WriteString(mat.roughnessPath);
            aWriter.WriteString(mat.metallicPath);
        }

        NL_TRACE(GCoreLogger, "wrote {} vertices, {} indices", aRawData.vertices.size(), aRawData.indices.size());
    }

    RawMeshData MeshSerializer::Read(BinaryReader& aReader)
    {
        NL_SCOPE_CORE("MeshSerializer");
        RawMeshData data{};

        // Bounds
        data.boundsMin[0] = aReader.Read<float>();
        data.boundsMin[1] = aReader.Read<float>();
        data.boundsMin[2] = aReader.Read<float>();
        data.boundsMax[0] = aReader.Read<float>();
        data.boundsMax[1] = aReader.Read<float>();
        data.boundsMax[2] = aReader.Read<float>();

        // Submeshes
        const uint32_t submeshCount{ aReader.Read<uint32_t>() };
        data.submeshes.resize(submeshCount);
        for (auto& submesh : data.submeshes)
        {
            submesh.name         = aReader.ReadString();
            submesh.vertexOffset = aReader.Read<uint32_t>();
            submesh.vertexCount  = aReader.Read<uint32_t>();
            submesh.indexOffset  = aReader.Read<uint32_t>();
            submesh.indexCount   = aReader.Read<uint32_t>();
            submesh.materialIndex = aReader.Read<int32_t>();
        }

        // Vertices
        const uint32_t vertexCount{ aReader.Read<uint32_t>() };
        data.vertices.resize(vertexCount);
        aReader.ReadBytes(std::span(reinterpret_cast<uint8_t*>(data.vertices.data()), vertexCount * sizeof(RawVertex)));

        // Indices
        const uint32_t indexCount{ aReader.Read<uint32_t>() };
        data.indices.resize(indexCount);
        aReader.ReadBytes(std::span(reinterpret_cast<uint8_t*>(data.indices.data()), indexCount * sizeof(uint32_t)));

        // Materials
        const uint32_t materialCount{ aReader.Read<uint32_t>() };
        data.materials.resize(materialCount);
        for (auto& mat : data.materials)
        {
            mat.name = aReader.ReadString();
            aReader.ReadBytes(std::span(reinterpret_cast<uint8_t*>(mat.baseColor), sizeof(mat.baseColor)));
            mat.roughness     = aReader.Read<float>();
            mat.metallic      = aReader.Read<float>();
            mat.albedoPath    = aReader.ReadString();
            mat.normalPath    = aReader.ReadString();
            mat.roughnessPath = aReader.ReadString();
            mat.metallicPath  = aReader.ReadString();
        }

        NL_TRACE(GCoreLogger, "read {} vertices, {} indices", data.vertices.size(), data.indices.size());
        return data;
    }
}