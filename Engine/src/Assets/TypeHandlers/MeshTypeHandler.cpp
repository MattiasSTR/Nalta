#include "npch.h"
#include "Nalta/Assets/TypeHandlers/MeshTypeHandler.h"

#include "Nalta/Assets/AssetManager.h"
#include "Nalta/Assets/RawAssetData.h"
#include "Nalta/Assets/Importers/IAssetImporter.h"
#include "Nalta/Assets/Processors/MeshProcessor.h"
#include "Nalta/Assets/Serializers/MeshSerializer.h"
#include "Nalta/Core/BinaryIO.h"
#include "Nalta/Graphics/GPUResourceManager.h"

namespace Nalta
{
    bool MeshTypeHandler::CookAndProcess(const uint64_t aHash, const AssetPath& aPath, AssetRegistry& aRegistry)
    {
        const IAssetImporter* importer{ myImporters.GetImporter(aPath) };
        if (importer == nullptr)
        {
            NL_ERROR(GCoreLogger, "no importer for '{}'", aPath.GetPath());
            return false;
        }

        const auto rawBase{ importer->Import(aPath) };
        if (!rawBase || !rawBase->IsValid())
        {
            NL_ERROR(GCoreLogger, "import failed for '{}'", aPath.GetPath());
            return false;
        }

        auto& raw{ static_cast<RawMeshData&>(*rawBase) };
        const MeshKey key{ myStore.GetKey(aHash) };

        bool processOk{ false };
        myStore.Modify(key, [&](Mesh& aMesh)
        {
            processOk = MeshProcessor::Process(raw, aMesh, *myGPU);
        });

        if (!processOk)
        {
            NL_ERROR(GCoreLogger, "mesh processing failed for '{}'", aPath.GetPath());
            return false;
        }

#ifndef N_SHIPPING
        BinaryWriter writer;
        AssetManager::WriteCookedHeader(writer, AssetType::Mesh);
        MeshSerializer::Write(raw, writer);

        const auto cookedPath{ AssetManager::GetCookedPath(aPath) };
        if (writer.SaveToFile(cookedPath))
        {
            AssetRegistryEntry entry;
            entry.sourcePath   = aPath.GetPath();
            entry.cookedFile   = cookedPath.filename().string();
            entry.assetType    = "Mesh";
            entry.dependencies = {};

            if (std::filesystem::exists(aPath.GetPath()))
            {
                entry.lastModified = std::filesystem::last_write_time(aPath.GetPath());
            }

            aRegistry.Register(entry);
            aRegistry.Save();
            NL_INFO(GCoreLogger, "cooked mesh to '{}'", cookedPath.string());
        }
#endif

        NL_INFO(GCoreLogger, "loaded mesh '{}'", aPath.GetPath());
        return true;
    }

    bool MeshTypeHandler::LoadFromCooked(uint64_t aHash, const AssetPath& aSourcePath, const std::filesystem::path& aCookedPath)
    {
        auto reader{ BinaryReader::FromFile(aCookedPath) };
        if (!reader.IsValid())
        {
            return false;
        }

        AssetType type{};
        if (!AssetManager::ReadCookedHeader(reader, type) || type != AssetType::Mesh)
        {
            NL_WARN(GCoreLogger, "cooked header mismatch for mesh '{}'", aCookedPath.string());
            return false;
        }

        auto raw{ MeshSerializer::Read(reader) };
        if (!raw.IsValid())
        {
            return false;
        }

        raw.sourcePath = aSourcePath;  // restore for debug naming

        const MeshKey key{ myStore.GetKey(aHash) };
        bool processOk{ false };
        myStore.Modify(key, [&](Mesh& aMesh)
        {
            processOk = MeshProcessor::Process(raw, aMesh, *myGPU);
        });

        if (!processOk)
        {
            NL_ERROR(GCoreLogger, "mesh processing failed for '{}'", aCookedPath.string());
            return false;
        }

        NL_INFO(GCoreLogger, "loaded mesh from cooked '{}'", aCookedPath.string());
        return true;
    }

    bool MeshTypeHandler::IsUploadComplete(const uint64_t aHash) const
    {
        const MeshKey key{ myStore.GetKey(aHash) };
        bool vbReady{ false };
        bool ibReady{ false };
        myStore.Peek(key, [&](const Mesh& aMesh)
        {
            vbReady = myGPU->IsBufferReady(aMesh.vertexBuffer);
            ibReady = myGPU->IsBufferReady(aMesh.indexBuffer);
        });
        return vbReady && ibReady;
    }

    bool MeshTypeHandler::OwnsHash(const uint64_t aHash) const
    {
        return myStore.Contains(aHash);
    }

    void MeshTypeHandler::SetReady(const uint64_t aHash)
    {
        myStore.SetState(myStore.GetKey(aHash), AssetState::Ready);
    }

    void MeshTypeHandler::SetFailed(const uint64_t aHash)
    {
        myStore.SetState(myStore.GetKey(aHash), AssetState::Failed);
    }

    AssetState MeshTypeHandler::GetState(const uint64_t aHash) const
    {
        return myStore.GetState(myStore.GetKey(aHash));
    }

    void MeshTypeHandler::InitializeFallback()
    {
        const std::vector<MeshVertex> vertices
        {
            // Front
            { .position = float3{-0.5f, -0.5f, -0.5f}, .normal = float3{ 0.f,  0.f, -1.f}, .tangent = float4{1.f, 0.f, 0.f, 1.f}, .texCoord = float2{0.f, 1.f} },
            { .position = float3{ 0.5f, -0.5f, -0.5f}, .normal = float3{ 0.f,  0.f, -1.f}, .tangent = float4{1.f, 0.f, 0.f, 1.f}, .texCoord = float2{1.f, 1.f} },
            { .position = float3{ 0.5f,  0.5f, -0.5f}, .normal = float3{ 0.f,  0.f, -1.f}, .tangent = float4{1.f, 0.f, 0.f, 1.f}, .texCoord = float2{1.f, 0.f} },
            { .position = float3{-0.5f,  0.5f, -0.5f}, .normal = float3{ 0.f,  0.f, -1.f}, .tangent = float4{1.f, 0.f, 0.f, 1.f}, .texCoord = float2{0.f, 0.f} },
            // Back
            { .position = float3{ 0.5f, -0.5f,  0.5f}, .normal = float3{ 0.f,  0.f,  1.f}, .tangent = float4{-1.f, 0.f, 0.f, 1.f}, .texCoord = float2{0.f, 1.f} },
            { .position = float3{-0.5f, -0.5f,  0.5f}, .normal = float3{ 0.f,  0.f,  1.f}, .tangent = float4{-1.f, 0.f, 0.f, 1.f}, .texCoord = float2{1.f, 1.f} },
            { .position = float3{-0.5f,  0.5f,  0.5f}, .normal = float3{ 0.f,  0.f,  1.f}, .tangent = float4{-1.f, 0.f, 0.f, 1.f}, .texCoord = float2{1.f, 0.f} },
            { .position = float3{ 0.5f,  0.5f,  0.5f}, .normal = float3{ 0.f,  0.f,  1.f}, .tangent = float4{-1.f, 0.f, 0.f, 1.f}, .texCoord = float2{0.f, 0.f} },
            // Left
            { .position = float3{-0.5f, -0.5f,  0.5f}, .normal = float3{-1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f, -1.f, 1.f}, .texCoord = float2{0.f, 1.f} },
            { .position = float3{-0.5f, -0.5f, -0.5f}, .normal = float3{-1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f, -1.f, 1.f}, .texCoord = float2{1.f, 1.f} },
            { .position = float3{-0.5f,  0.5f, -0.5f}, .normal = float3{-1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f, -1.f, 1.f}, .texCoord = float2{1.f, 0.f} },
            { .position = float3{-0.5f,  0.5f,  0.5f}, .normal = float3{-1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f, -1.f, 1.f}, .texCoord = float2{0.f, 0.f} },
            // Right
            { .position = float3{ 0.5f, -0.5f, -0.5f}, .normal = float3{ 1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f,  1.f, 1.f}, .texCoord = float2{0.f, 1.f} },
            { .position = float3{ 0.5f, -0.5f,  0.5f}, .normal = float3{ 1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f,  1.f, 1.f}, .texCoord = float2{1.f, 1.f} },
            { .position = float3{ 0.5f,  0.5f,  0.5f}, .normal = float3{ 1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f,  1.f, 1.f}, .texCoord = float2{1.f, 0.f} },
            { .position = float3{ 0.5f,  0.5f, -0.5f}, .normal = float3{ 1.f,  0.f,  0.f}, .tangent = float4{0.f, 0.f,  1.f, 1.f}, .texCoord = float2{0.f, 0.f} },
            // Top
            { .position = float3{-0.5f,  0.5f, -0.5f}, .normal = float3{ 0.f,  1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{0.f, 1.f} },
            { .position = float3{ 0.5f,  0.5f, -0.5f}, .normal = float3{ 0.f,  1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{1.f, 1.f} },
            { .position = float3{ 0.5f,  0.5f,  0.5f}, .normal = float3{ 0.f,  1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{1.f, 0.f} },
            { .position = float3{-0.5f,  0.5f,  0.5f}, .normal = float3{ 0.f,  1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{0.f, 0.f} },
            // Bottom
            { .position = float3{-0.5f, -0.5f,  0.5f}, .normal = float3{ 0.f, -1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{0.f, 1.f} },
            { .position = float3{ 0.5f, -0.5f,  0.5f}, .normal = float3{ 0.f, -1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{1.f, 1.f} },
            { .position = float3{ 0.5f, -0.5f, -0.5f}, .normal = float3{ 0.f, -1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{1.f, 0.f} },
            { .position = float3{-0.5f, -0.5f, -0.5f}, .normal = float3{ 0.f, -1.f,  0.f}, .tangent = float4{1.f, 0.f,  0.f, 1.f}, .texCoord = float2{0.f, 0.f} },
        };

        const std::vector<uint32_t> indices
        {
             0,  2,  1,  2,  0,  3,  // front
             4,  6,  5,  6,  4,  7,  // back
             8, 10,  9, 10,  8, 11,  // left
            12, 14, 13, 14, 12, 15,  // right
            16, 18, 17, 18, 16, 19,  // top
            20, 22, 21, 22, 20, 23,  // bottom
        };

        RawMeshData raw;
        raw.vertices.reserve(vertices.size());
        for (const auto& v : vertices)
        {
            raw.vertices.push_back(
            {
                .position = { v.position.x, v.position.y, v.position.z },
                .normal   = { v.normal.x,   v.normal.y,   v.normal.z   },
                .texCoord = { v.texCoord.x, v.texCoord.y               },
                .tangent  = { v.tangent.x,  v.tangent.y,  v.tangent.z, v.tangent.w }
            });
        }
        raw.indices      = indices;
        raw.boundsMin[0] = raw.boundsMin[1] = raw.boundsMin[2] = -0.5f;
        raw.boundsMax[0] = raw.boundsMax[1] = raw.boundsMax[2] =  0.5f;
        raw.submeshes.push_back(
        {
            .name          = "fallback",
            .vertexOffset  = 0,
            .vertexCount   = static_cast<uint32_t>(vertices.size()),
            .indexOffset   = 0,
            .indexCount    = static_cast<uint32_t>(indices.size()),
            .materialIndex = 0
        });
        raw.sourcePath = AssetPath{ "fallback" };

        const bool ok{ MeshProcessor::Process(raw, myFallback, *myGPU) };
        N_CORE_ASSERT(ok, "failed to create fallback mesh");

        myGPU->GetDevice().FlushUploads();
        myFallback.state = AssetState::Ready;

        myStore.SetFallback(&myFallback);
        NL_INFO(GCoreLogger, "fallback mesh created");
    }

    void MeshTypeHandler::DestroyAll(Graphics::GPUResourceManager& aGPU)
    {
        myStore.ForEach([&aGPU](const Mesh& aMesh)
        {
            if (aMesh.vertexBuffer.IsValid())
            {
                aGPU.DestroyBuffer(aMesh.vertexBuffer);
            }
            if (aMesh.indexBuffer.IsValid())
            {
                aGPU.DestroyBuffer(aMesh.indexBuffer);
            }
        });
        myStore.Clear();
    }
}
