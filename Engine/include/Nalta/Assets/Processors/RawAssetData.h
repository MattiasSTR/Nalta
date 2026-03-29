#pragma once
#include "Nalta/Assets/AssetPath.h"
#include "Nalta/Graphics/Shader/ShaderStage.h"

#include <vector>
#include <string>
#include <cstdint>

namespace Nalta
{
    struct RawVertex
    {
        float position[3];
        float normal[3];
        float texCoord[2];
        float tangent[4]; // xyz tangent, w = bitangent sign
    };

    struct RawSubmesh
    {
        std::string name;
        uint32_t vertexOffset{ 0 };
        uint32_t vertexCount{ 0 };
        uint32_t indexOffset{ 0 };
        uint32_t indexCount{ 0 };
        int32_t materialIndex{ -1 };
    };

    struct RawMaterial
    {
        std::string name;
        float baseColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
        float roughness{ 0.5f };
        float metallic{ 0.0f };
        std::string albedoPath;
        std::string normalPath;
        std::string roughnessPath;
        std::string metallicPath;
    };
    
    struct RawAssetData
    {
        AssetPath sourcePath;
        virtual ~RawAssetData() = default;
        [[nodiscard]] virtual bool IsValid() const { return true; }
    };

    // Intermediate representation - file-type agnostic
    // All importers produce this, all processors consume it
    struct RawMeshData : RawAssetData
    {
        std::vector<RawVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<RawSubmesh> submeshes;
        std::vector<RawMaterial> materials;

        // Bounds - computed during import
        float boundsMin[3]{ 0.0f, 0.0f, 0.0f };
        float boundsMax[3]{ 0.0f, 0.0f, 0.0f };

        [[nodiscard]] bool IsValid() const override
        {
            return !vertices.empty() && !indices.empty();
        }
    };
    
    struct RawShaderStageData
    {
        Graphics::ShaderStage stage{};
        std::vector<uint8_t> bytecode;
        std::vector<uint8_t> reflection;
    };
    
    struct RawPipelineData : RawAssetData
    {
        // Shader source - compiled during import
        std::string shaderPath;
        std::string vertexEntry{ "VSMain" };
        std::string pixelEntry { "PSMain" };
        std::unordered_map<std::string, std::string> defines;

        // Compiled bytecode per stage — filled by importer
        std::vector<RawShaderStageData> stages;

        // Pipeline state
        std::string cullMode{ "Back" };
        std::string fillMode{ "Solid" };
        bool depthEnabled{ false };
        bool depthWrite  { false };
        std::string depthCompare{ "Greater" };
        bool blendEnabled{ false };

        [[nodiscard]] bool IsValid() const override
        {
            return !stages.empty();
        }
    };
}