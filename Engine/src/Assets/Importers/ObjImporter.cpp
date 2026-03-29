#include "npch.h"
#include "Nalta/Assets/Importers/ObjImporter.h"
#include "Nalta/Assets/Processors/RawAssetData.h"
#include "Nalta/Assets/AssetPath.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

namespace Nalta
{
    namespace
    {
        struct ObjVertex
        {
            uint32_t positionIndex{ 0 };
            uint32_t texCoordIndex{ 0 };
            uint32_t normalIndex{ 0 };

            bool operator==(const ObjVertex& aOther) const
            {
                return positionIndex == aOther.positionIndex && texCoordIndex == aOther.texCoordIndex && normalIndex   == aOther.normalIndex;
            }
        };
        
        struct ObjVertexHash
        {
            size_t operator()(const ObjVertex& aVertex) const
            {
                size_t hash{ 0 };
                hash ^= std::hash<uint32_t>{}(aVertex.positionIndex) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                hash ^= std::hash<uint32_t>{}(aVertex.texCoordIndex) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                hash ^= std::hash<uint32_t>{}(aVertex.normalIndex)   + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                return hash;
            }
        };
        
        void ComputeBounds(RawMeshData& aData)
        {
            if (aData.vertices.empty())
            {
                return;
            }

            aData.boundsMin[0] = aData.vertices[0].position[0];
            aData.boundsMin[1] = aData.vertices[0].position[1];
            aData.boundsMin[2] = aData.vertices[0].position[2];
            aData.boundsMax[0] = aData.boundsMin[0];
            aData.boundsMax[1] = aData.boundsMin[1];
            aData.boundsMax[2] = aData.boundsMin[2];

            for (const auto& vertex : aData.vertices)
            {
                for (int i{ 0 }; i < 3; ++i)
                {
                    aData.boundsMin[i] = std::min(aData.boundsMin[i], vertex.position[i]);
                    aData.boundsMax[i] = std::max(aData.boundsMax[i], vertex.position[i]);
                }
            }
        }
        
        void ComputeFlatNormals(RawMeshData& aData)
        {
            // Zero out all normals first
            for (auto& vertex : aData.vertices)
            {
                vertex.normal[0] = 0.0f;
                vertex.normal[1] = 0.0f;
                vertex.normal[2] = 0.0f;
            }
            
            // Accumulate face normals
            for (size_t i{ 0 }; i < aData.indices.size(); i += 3)
            {
                const uint32_t i0{ aData.indices[i + 0] };
                const uint32_t i1{ aData.indices[i + 1] };
                const uint32_t i2{ aData.indices[i + 2] };

                const float* p0{ aData.vertices[i0].position };
                const float* p1{ aData.vertices[i1].position };
                const float* p2{ aData.vertices[i2].position };

                const float e0[3]{ p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
                const float e1[3]{ p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };

                const float normal[3]
                {
                    e0[1] * e1[2] - e0[2] * e1[1],
                    e0[2] * e1[0] - e0[0] * e1[2],
                    e0[0] * e1[1] - e0[1] * e1[0]
                };

                for (const uint32_t idx : { i0, i1, i2 })
                {
                    aData.vertices[idx].normal[0] += normal[0];
                    aData.vertices[idx].normal[1] += normal[1];
                    aData.vertices[idx].normal[2] += normal[2];
                }
            }
            
            // Normalize
            for (auto& vertex : aData.vertices)
            {
                const float len{ std::sqrt(
                    vertex.normal[0] * vertex.normal[0] +
                    vertex.normal[1] * vertex.normal[1] +
                    vertex.normal[2] * vertex.normal[2]) };

                if (len > 0.0001f)
                {
                    vertex.normal[0] /= len;
                    vertex.normal[1] /= len;
                    vertex.normal[2] /= len;
                }
            }
        }
        
        void ComputeTangents(RawMeshData& aData)
        {
            const size_t vertexCount{ aData.vertices.size() };
            
            std::vector<std::array<float, 3>> tan1(vertexCount, { 0.0f, 0.0f, 0.0f });
            std::vector<std::array<float, 3>> tan2(vertexCount, { 0.0f, 0.0f, 0.0f });
        
            // Accumulate per-triangle tangents
            for (size_t i{ 0 }; i < aData.indices.size(); i += 3)
            {
                const uint32_t i0{ aData.indices[i + 0] };
                const uint32_t i1{ aData.indices[i + 1] };
                const uint32_t i2{ aData.indices[i + 2] };
        
                const RawVertex& v0{ aData.vertices[i0] };
                const RawVertex& v1{ aData.vertices[i1] };
                const RawVertex& v2{ aData.vertices[i2] };
        
                const float x1{ v1.position[0] - v0.position[0] };
                const float x2{ v2.position[0] - v0.position[0] };
                const float y1{ v1.position[1] - v0.position[1] };
                const float y2{ v2.position[1] - v0.position[1] };
                const float z1{ v1.position[2] - v0.position[2] };
                const float z2{ v2.position[2] - v0.position[2] };
        
                const float s1{ v1.texCoord[0] - v0.texCoord[0] };
                const float s2{ v2.texCoord[0] - v0.texCoord[0] };
                const float t1{ v1.texCoord[1] - v0.texCoord[1] };
                const float t2{ v2.texCoord[1] - v0.texCoord[1] };
        
                const float denom{ s1 * t2 - s2 * t1 };
                if (std::abs(denom) < 0.0001f) continue;
        
                const float r{ 1.0f / denom };
        
                const float tx{ (t2 * x1 - t1 * x2) * r };
                const float ty{ (t2 * y1 - t1 * y2) * r };
                const float tz{ (t2 * z1 - t1 * z2) * r };
        
                const float bx{ (s1 * x2 - s2 * x1) * r };
                const float by{ (s1 * y2 - s2 * y1) * r };
                const float bz{ (s1 * z2 - s2 * z1) * r };
        
                for (const uint32_t idx : { i0, i1, i2 })
                {
                    tan1[idx][0] += tx;
                    tan1[idx][1] += ty;
                    tan1[idx][2] += tz;
                    tan2[idx][0] += bx;
                    tan2[idx][1] += by;
                    tan2[idx][2] += bz;
                }
            }
        
            // Orthogonalize and compute handedness
            for (size_t i{ 0 }; i < vertexCount; ++i)
            {
                const float* n{ aData.vertices[i].normal };
                const float* t{ tan1[i].data() };
                const float* b{ tan2[i].data() };
        
                // Gram-Schmidt orthogonalize
                const float dot{ n[0] * t[0] + n[1] * t[1] + n[2] * t[2] };
        
                float tangent[3]
                {
                    t[0] - n[0] * dot,
                    t[1] - n[1] * dot,
                    t[2] - n[2] * dot
                };
        
                // Normalize
                const float len{ std::sqrt(
                    tangent[0] * tangent[0] +
                    tangent[1] * tangent[1] +
                    tangent[2] * tangent[2]) };
        
                if (len > 0.0001f)
                {
                    tangent[0] /= len;
                    tangent[1] /= len;
                    tangent[2] /= len;
                }
        
                // Handedness - cross(n, t) dot b
                // w = 1 if right-handed, -1 if left-handed
                const float cx{ n[1] * tangent[2] - n[2] * tangent[1] };
                const float cy{ n[2] * tangent[0] - n[0] * tangent[2] };
                const float cz{ n[0] * tangent[1] - n[1] * tangent[0] };
        
                const float handedness{(cx * b[0] + cy * b[1] + cz * b[2]) < 0.0f ? -1.0f : 1.0f };
        
                aData.vertices[i].tangent[0] = tangent[0];
                aData.vertices[i].tangent[1] = tangent[1];
                aData.vertices[i].tangent[2] = tangent[2];
                aData.vertices[i].tangent[3] = handedness;
            }
        }
    }
    
    bool ObjImporter::CanImport(const std::string& aExtension) const
    {
        return aExtension == ".obj" || aExtension == ".OBJ";
    }
    
    std::unique_ptr<RawAssetData> ObjImporter::Import(const AssetPath& aPath) const
    {
        NL_SCOPE_CORE("ObjImporter");
        NL_TRACE(GCoreLogger, "importing '{}'", aPath.GetPath());
        
        std::ifstream file{ aPath.GetPath() };
        if (!file.is_open())
        {
            NL_ERROR(GCoreLogger, "failed to open '{}'", aPath.GetPath());
            return nullptr;
        }

        std::vector<float[3]> positions;
        std::vector<float[3]> normals;
        std::vector<float[2]> texCoords;

        // Use vectors of floats instead of arrays for easier management
        std::vector<std::array<float, 3>> posArray;
        std::vector<std::array<float, 3>> normArray;
        std::vector<std::array<float, 2>> uvArray;

        auto result{ std::make_unique<RawMeshData>() };
        result->sourcePath = aPath;

        std::unordered_map<ObjVertex, uint32_t, ObjVertexHash> vertexMap;
        
        std::string currentGroup{ "default" };
        uint32_t groupIndexStart{ 0 };

        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            std::istringstream ss{ line };
            std::string token;
            ss >> token;

            if (token == "v")
            {
                auto& pos{ posArray.emplace_back() };
                ss >> pos[0] >> pos[1] >> pos[2];
            }
            else if (token == "vn")
            {
                auto& norm{ normArray.emplace_back() };
                ss >> norm[0] >> norm[1] >> norm[2];
            }
            else if (token == "vt")
            {
                auto& uv{ uvArray.emplace_back() };
                ss >> uv[0] >> uv[1];
                uv[1] = 1.0f - uv[1]; // flip V — OBJ is bottom-left origin, DX12 is top-left
            }
            else if (token == "g" || token == "o")
            {
                // New group/object — record previous submesh if it had faces
                const uint32_t currentCount{ static_cast<uint32_t>(result->indices.size()) };
                if (currentCount > groupIndexStart)
                {
                    RawSubmesh submesh{};
                    submesh.name        = currentGroup;
                    submesh.indexOffset = groupIndexStart;
                    submesh.indexCount  = currentCount - groupIndexStart;
                    result->submeshes.push_back(submesh);
                    groupIndexStart = currentCount;
                }
                ss >> currentGroup;
            }
            else if (token == "f")
            {
                // Parse face — supports triangles and quads, fan triangulation for n-gons
                std::vector<uint32_t> faceIndices;
                std::string faceToken;

                while (ss >> faceToken)
                {
                    ObjVertex vertex{};

                    // Count slashes to determine format
                    const size_t firstSlash{ faceToken.find('/') };
                    const size_t secondSlash{ firstSlash != std::string::npos ? faceToken.find('/', firstSlash + 1) : std::string::npos };

                    if (firstSlash == std::string::npos)
                    {
                        // v only
                        vertex.positionIndex = std::stoul(faceToken) - 1;
                    }
                    else if (secondSlash == std::string::npos)
                    {
                        // v/vt
                        vertex.positionIndex = std::stoul(faceToken.substr(0, firstSlash)) - 1;
                        vertex.texCoordIndex = std::stoul(faceToken.substr(firstSlash + 1)) - 1;
                    }
                    else if (secondSlash == firstSlash + 1)
                    {
                        // v//vn — no texcoord
                        vertex.positionIndex = std::stoul(faceToken.substr(0, firstSlash)) - 1;
                        vertex.normalIndex   = std::stoul(faceToken.substr(secondSlash + 1)) - 1;
                    }
                    else
                    {
                        // v/vt/vn
                        vertex.positionIndex = std::stoul(faceToken.substr(0, firstSlash)) - 1;
                        vertex.texCoordIndex = std::stoul(faceToken.substr(firstSlash + 1, secondSlash - firstSlash - 1)) - 1;
                        vertex.normalIndex   = std::stoul(faceToken.substr(secondSlash + 1)) - 1;
                    }


                    const auto it{ vertexMap.find(vertex) };
                    if (it != vertexMap.end())
                    {
                        faceIndices.push_back(it->second);
                    }
                    else
                    {
                        const uint32_t newIndex{ static_cast<uint32_t>(result->vertices.size()) };
                        vertexMap[vertex] = newIndex;
                        faceIndices.push_back(newIndex);

                        RawVertex rawVertex{};

                        if (vertex.positionIndex < posArray.size())
                        {
                            rawVertex.position[0] = posArray[vertex.positionIndex][0];
                            rawVertex.position[1] = posArray[vertex.positionIndex][1];
                            rawVertex.position[2] = posArray[vertex.positionIndex][2];
                        }

                        if (!normArray.empty() && vertex.normalIndex < normArray.size())
                        {
                            rawVertex.normal[0] = normArray[vertex.normalIndex][0];
                            rawVertex.normal[1] = normArray[vertex.normalIndex][1];
                            rawVertex.normal[2] = normArray[vertex.normalIndex][2];
                        }

                        if (!uvArray.empty() && vertex.texCoordIndex < uvArray.size())
                        {
                            rawVertex.texCoord[0] = uvArray[vertex.texCoordIndex][0];
                            rawVertex.texCoord[1] = uvArray[vertex.texCoordIndex][1];
                        }

                        result->vertices.push_back(rawVertex);
                    }
                }

                // Fan triangulation - works for triangles, quads, and n-gons
                for (size_t i{ 1 }; i + 1 < faceIndices.size(); ++i)
                {
                    result->indices.push_back(faceIndices[0]);
                    result->indices.push_back(faceIndices[i]);
                    result->indices.push_back(faceIndices[i + 1]);
                }
            }
            else if (token == "usemtl")
            {
                // Material reference - store name for later
                std::string matName;
                ss >> matName;
                RawMaterial mat{};
                mat.name = matName;
                result->materials.push_back(mat);
            }
        }
        
        // Add final submesh
        const uint32_t finalCount{ static_cast<uint32_t>(result->indices.size()) };
        if (finalCount > groupIndexStart)
        {
            RawSubmesh submesh{};
            submesh.name        = currentGroup;
            submesh.indexOffset = groupIndexStart;
            submesh.indexCount  = finalCount - groupIndexStart;
            result->submeshes.push_back(submesh);
        }
        
        // If no normals in file compute flat normals
        if (normArray.empty())
        {
            NL_TRACE(GCoreLogger, "no normals in '{}' — computing flat normals", aPath.GetPath());
            ComputeFlatNormals(*result);
        }

        ComputeTangents(*result);
        ComputeBounds(*result);

        NL_INFO(GCoreLogger, "imported '{}' ({} vertices, {} indices, {} submeshes)",
            aPath.GetPath(),
            result->vertices.size(),
            result->indices.size(),
            result->submeshes.size());

        return result;
    }
    
}