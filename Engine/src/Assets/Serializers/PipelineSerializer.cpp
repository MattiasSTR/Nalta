#include "npch.h"
#include "Nalta/Assets/Serializers/PipelineSerializer.h"
#include "Nalta/Assets/RawAssetData.h"
#include "Nalta/Core/BinaryIO.h"

namespace Nalta
{
    void PipelineSerializer::Write(const RawPipelineData& aRawData, BinaryWriter& aWriter)
    {
        NL_SCOPE_CORE("PipelineSerializer");

        // Pipeline state strings
        aWriter.WriteString(aRawData.shaderPath);
        aWriter.WriteString(aRawData.vertexEntry);
        aWriter.WriteString(aRawData.pixelEntry);
        aWriter.WriteString(aRawData.cullMode);
        aWriter.WriteString(aRawData.fillMode);
        aWriter.WriteString(aRawData.depthCompare);
        aWriter.Write(aRawData.depthEnabled);
        aWriter.Write(aRawData.depthWrite);
        aWriter.Write(aRawData.blendEnabled);
        
        // Defines
        aWriter.Write(static_cast<uint32_t>(aRawData.defines.size()));
        for (const auto& [key, value] : aRawData.defines)
        {
            aWriter.WriteString(key);
            aWriter.WriteString(value);
        }

        // Compiled shader stages
        aWriter.Write(static_cast<uint32_t>(aRawData.stages.size()));
        for (const auto& stage : aRawData.stages)
        {
            aWriter.Write(static_cast<uint8_t>(stage.stage));

            aWriter.Write(static_cast<uint32_t>(stage.bytecode.size()));
            aWriter.WriteBytes(std::span(stage.bytecode));

            aWriter.Write(static_cast<uint32_t>(stage.reflection.size()));
            aWriter.WriteBytes(std::span(stage.reflection));
        }

        NL_TRACE(GCoreLogger, "wrote {} stages", aRawData.stages.size());
    }

    RawPipelineData PipelineSerializer::Read(BinaryReader& aReader)
    {
        NL_SCOPE_CORE("PipelineSerializer");
        RawPipelineData data{};

        data.shaderPath = aReader.ReadString();
        data.vertexEntry = aReader.ReadString();
        data.pixelEntry = aReader.ReadString();
        data.cullMode = aReader.ReadString();
        data.fillMode = aReader.ReadString();
        data.depthCompare = aReader.ReadString();
        data.depthEnabled = aReader.Read<bool>();
        data.depthWrite = aReader.Read<bool>();
        data.blendEnabled = aReader.Read<bool>();
        
        const uint32_t defineCount{ aReader.Read<uint32_t>() };
        for (uint32_t i{ 0 }; i < defineCount; ++i)
        {
            const std::string key{ aReader.ReadString() };
            const std::string value{ aReader.ReadString() };
            data.defines[key] = value;
        }

        const uint32_t stageCount{ aReader.Read<uint32_t>() };
        data.stages.resize(stageCount);
        for (auto& stage : data.stages)
        {
            stage.stage = static_cast<Graphics::ShaderStage>(aReader.Read<uint8_t>());

            const uint32_t bytecodeSize{ aReader.Read<uint32_t>() };
            stage.bytecode.resize(bytecodeSize);
            aReader.ReadBytes(std::span(stage.bytecode));

            const uint32_t reflectionSize{ aReader.Read<uint32_t>() };
            stage.reflection.resize(reflectionSize);
            aReader.ReadBytes(std::span(stage.reflection));
        }

        NL_TRACE(GCoreLogger, "read {} stages", data.stages.size());
        return data;
    }
}