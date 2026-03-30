#include "npch.h"
#include "Nalta/Assets/Processors/PipelineProcessor.h"
#include "Nalta/Assets/RawAssetData.h"
#include "Nalta/Assets/Pipeline.h"
#include "Nalta/Graphics/GraphicsSystem.h"
#include "Nalta/Graphics/Pipeline/PipelineDesc.h"
#include "Nalta/Graphics/Shader/Shader.h"

namespace Nalta
{
    bool PipelineProcessor::Process(const RawPipelineData& aRawData, Pipeline& outPipeline, GraphicsSystem& aGraphicsSystem)
    {
        NL_SCOPE_CORE("PipelineProcessor");

        // Reconstruct Shader from bytecode
        auto shader{ std::make_shared<Graphics::Shader>() };
        for (const auto& stageData : aRawData.stages)
        {
            Graphics::ShaderBytecode bytecode;
            bytecode.code       = stageData.bytecode;
            bytecode.reflection = stageData.reflection;
            shader->SetBytecode(stageData.stage, std::move(bytecode));
        }

        // Build PipelineDesc
        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = shader;
        pipelineDesc.rasterizer.cullMode = Graphics::CullModeFromString(aRawData.cullMode);
        pipelineDesc.rasterizer.fillMode = Graphics::FillModeFromString(aRawData.fillMode);
        pipelineDesc.depth.depthEnabled  = aRawData.depthEnabled;
        pipelineDesc.depth.depthWrite    = aRawData.depthWrite;
        pipelineDesc.depth.compareFunc   = Graphics::DepthCompareFromString(aRawData.depthCompare);
        pipelineDesc.blend.blendEnabled  = aRawData.blendEnabled;

        outPipeline.gpuHandle = aGraphicsSystem.CreatePipeline(pipelineDesc);

        if (!outPipeline.gpuHandle.IsValid())
        {
            NL_ERROR(GCoreLogger, "failed to create pipeline for '{}'", aRawData.sourcePath.IsEmpty() ? "cooked" : aRawData.sourcePath.GetPath());
            return false;
        }

        NL_INFO(GCoreLogger, "created pipeline for '{}'", aRawData.sourcePath.IsEmpty() ? "cooked" : aRawData.sourcePath.GetPath());
        return true;
    }
}
