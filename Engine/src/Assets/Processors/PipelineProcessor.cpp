#include "npch.h"
#include "Nalta/Assets/Processors/PipelineProcessor.h"
#include "Nalta/Assets/Processors/RawAssetData.h"
#include "Nalta/Assets/Pipeline/PipelineAsset.h"
#include "Nalta/Graphics/GraphicsSystem.h"
#include "Nalta/Graphics/Pipeline/PipelineDesc.h"
#include "Nalta/Graphics/Shader/Shader.h"

namespace Nalta
{
    bool PipelineProcessor::Process(const RawAssetData& aBaseData, Asset& aOutAsset, GraphicsSystem& aGraphicsSystem) const
    {
        NL_SCOPE_CORE("PipelineProcessor");

        const auto& data{ static_cast<const RawPipelineData&>(aBaseData) };
        auto& pipeline{ static_cast<PipelineAsset&>(aOutAsset) };

        // Reconstruct Shader from bytecode
        auto shader{ std::make_shared<Graphics::Shader>() };
        for (const auto& stageData : data.stages)
        {
            Graphics::ShaderBytecode bytecode;
            bytecode.code       = stageData.bytecode;
            bytecode.reflection = stageData.reflection;
            shader->SetBytecode(stageData.stage, std::move(bytecode));
        }

        // Build PipelineDesc
        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = shader;
        pipelineDesc.rasterizer.cullMode = Graphics::CullModeFromString(data.cullMode);
        pipelineDesc.rasterizer.fillMode = Graphics::FillModeFromString(data.fillMode);
        pipelineDesc.depth.depthEnabled  = data.depthEnabled;
        pipelineDesc.depth.depthWrite    = data.depthWrite;
        pipelineDesc.depth.compareFunc   = Graphics::DepthCompareFromString(data.depthCompare);
        pipelineDesc.blend.blendEnabled  = data.blendEnabled;

        pipeline.myPipelineHandle = aGraphicsSystem.CreatePipeline(pipelineDesc);

        if (!pipeline.myPipelineHandle.IsValid())
        {
            NL_ERROR(GCoreLogger, "failed to create pipeline for '{}'", data.sourcePath.GetPath());
            return false;
        }

        NL_INFO(GCoreLogger, "created pipeline for '{}'", data.sourcePath.GetPath());
        return true;
    }
}