#include "npch.h"
#include "Nalta/Assets/Importers/PipelineImporter.h"
#include "Nalta/Assets/Processors/RawAssetData.h"
#include "Nalta/Graphics/Pipeline/PipelineDesc.h"
#include "Nalta/Graphics/Shader/ShaderCompiler.h"
#include "Nalta/Graphics/Shader/ShaderDesc.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Nalta
{
    PipelineImporter::PipelineImporter(Graphics::ShaderCompiler* aShaderCompiler)
        : myShaderCompiler(aShaderCompiler)
    {}

    bool PipelineImporter::CanImport(const std::string& aExtension) const
    {
        return aExtension == ".pipeline";
    }

    std::unique_ptr<RawAssetData> PipelineImporter::Import(const AssetPath& aPath) const
    {
        NL_SCOPE_CORE("PipelineImporter");
        NL_TRACE(GCoreLogger, "importing '{}'", aPath.GetPath());

        std::ifstream file{ aPath.GetPath() };
        if (!file.is_open())
        {
            NL_ERROR(GCoreLogger, "failed to open '{}'", aPath.GetPath());
            return nullptr;
        }

        json root;
        try { root = json::parse(file); }
        catch (const json::exception& e)
        {
            NL_ERROR(GCoreLogger, "JSON parse error in '{}': {}", aPath.GetPath(), e.what());
            return nullptr;
        }

        auto result{ std::make_unique<RawPipelineData>() };
        result->sourcePath = aPath;

        result->shaderPath = root.value("shader",      "");
        result->vertexEntry = root.value("vertexEntry", "VSMain");
        result->pixelEntry = root.value("pixelEntry",  "PSMain");
        result->cullMode = root.value("cull", CullModeToString(Graphics::CullMode::Back));
        result->fillMode = root.value("fill", FillModeToString(Graphics::FillMode::Solid));
        result->blendEnabled = root.value("blend", false);

        if (root.contains("depth"))
        {
            const auto& depth{ root["depth"] };
            result->depthEnabled = depth.value("enabled", false);
            result->depthWrite   = depth.value("write",   false);
            result->depthCompare = depth.value("compare", DepthCompareToString(Graphics::DepthCompare::Greater));
        }
        
        if (root.contains("defines"))
        {
            for (const auto& [key, value] : root["defines"].items())
            {
                result->defines[key] = value.get<std::string>();
            }
        }

        if (result->shaderPath.empty())
        {
            NL_ERROR(GCoreLogger, "no shader specified in '{}'", aPath.GetPath());
            return nullptr;
        }

        // Resolve shader path relative to pipeline file
        const auto pipelineDir{ std::filesystem::path(aPath.GetPath()).parent_path() };
        const auto shaderPath { pipelineDir / result->shaderPath };

        // Compile shader
        N_CORE_ASSERT(myShaderCompiler, "null shader compiler");

        Graphics::ShaderDesc shaderDesc;
        shaderDesc.filePath = shaderPath;
        shaderDesc.defines = result->defines;
        shaderDesc.stages   =
        {
            { Graphics::ShaderStage::Vertex, result->vertexEntry },
            { Graphics::ShaderStage::Pixel,  result->pixelEntry  }
        };

        const auto shader{ myShaderCompiler->Compile(shaderDesc) };
        if (!shader)
        {
            NL_ERROR(GCoreLogger, "PipelineImporter: failed to compile shader '{}'", shaderPath.string());
            return nullptr;
        }

        // Store compiled bytecode in raw data
        for (const auto stage : { Graphics::ShaderStage::Vertex, Graphics::ShaderStage::Pixel })
        {
            if (!shader->HasStage(stage))
            {
                continue;
            }

            const auto& bytecode{ shader->GetBytecode(stage) };
            RawShaderStageData stageData;
            stageData.stage = stage;
            stageData.bytecode.assign(
                static_cast<const uint8_t*>(bytecode.GetData()),
                static_cast<const uint8_t*>(bytecode.GetData()) + bytecode.GetSize());
            stageData.reflection.assign(
                static_cast<const uint8_t*>(bytecode.GetReflectionData()),
                static_cast<const uint8_t*>(bytecode.GetReflectionData()) + bytecode.GetReflectionSize());

            result->stages.push_back(std::move(stageData));
        }

        NL_INFO(GCoreLogger, "imported '{}' ({} stages)", aPath.GetPath(), result->stages.size());
        return result;
    }
}