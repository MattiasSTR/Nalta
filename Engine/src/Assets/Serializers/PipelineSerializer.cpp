#include "npch.h"
#include "Nalta/Assets/Serializers/PipelineSerializer.h"
#include "Nalta/Assets/Processors/RawAssetData.h"
#include "Nalta/Core/BinaryIO.h"

namespace Nalta
{
    void PipelineSerializer::Write(const RawAssetData& aBaseData, BinaryWriter& aWriter) const
    {
        NL_SCOPE_CORE("PipelineSerializer");
        const auto& data{ static_cast<const RawPipelineData&>(aBaseData) };

        // Pipeline state strings
        aWriter.WriteString(data.shaderPath);
        aWriter.WriteString(data.vertexEntry);
        aWriter.WriteString(data.pixelEntry);
        aWriter.WriteString(data.cullMode);
        aWriter.WriteString(data.fillMode);
        aWriter.WriteString(data.depthCompare);
        aWriter.Write(data.depthEnabled);
        aWriter.Write(data.depthWrite);
        aWriter.Write(data.blendEnabled);

        // Compiled shader stages
        aWriter.Write(static_cast<uint32_t>(data.stages.size()));
        for (const auto& stage : data.stages)
        {
            aWriter.Write(static_cast<uint8_t>(stage.stage));

            aWriter.Write(static_cast<uint32_t>(stage.bytecode.size()));
            aWriter.WriteBytes(std::span(stage.bytecode));

            aWriter.Write(static_cast<uint32_t>(stage.reflection.size()));
            aWriter.WriteBytes(std::span(stage.reflection));
        }

        NL_TRACE(GCoreLogger, "wrote {} stages", data.stages.size());
    }

    std::unique_ptr<RawAssetData> PipelineSerializer::Read(BinaryReader& aReader) const
    {
        NL_SCOPE_CORE("PipelineSerializer");
        auto data{ std::make_unique<RawPipelineData>() };

        data->shaderPath   = aReader.ReadString();
        data->vertexEntry  = aReader.ReadString();
        data->pixelEntry   = aReader.ReadString();
        data->cullMode     = aReader.ReadString();
        data->fillMode     = aReader.ReadString();
        data->depthCompare = aReader.ReadString();
        data->depthEnabled = aReader.Read<bool>();
        data->depthWrite   = aReader.Read<bool>();
        data->blendEnabled = aReader.Read<bool>();

        const uint32_t stageCount{ aReader.Read<uint32_t>() };
        data->stages.resize(stageCount);
        for (auto& stage : data->stages)
        {
            stage.stage = static_cast<Graphics::ShaderStage>(aReader.Read<uint8_t>());

            const uint32_t bytecodeSize{ aReader.Read<uint32_t>() };
            stage.bytecode.resize(bytecodeSize);
            aReader.ReadBytes(std::span(stage.bytecode));

            const uint32_t reflectionSize{ aReader.Read<uint32_t>() };
            stage.reflection.resize(reflectionSize);
            aReader.ReadBytes(std::span(stage.reflection));
        }

        NL_TRACE(GCoreLogger, "read {} stages", data->stages.size());
        return data;
    }
}