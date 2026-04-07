#include "npch.h"
#include "Nalta/Graphics/ShaderBlobStore.h"
#include "Nalta/Core/BinaryIO.h"
#include "Nalta/RHI/Types/RHIShader.h"

namespace Nalta::Graphics
{
    std::optional<RHI::Shader> ShaderBlobStore::Load(const std::filesystem::path& aCachePath, const std::filesystem::path& aSourcePath)
    {
         if (!std::filesystem::exists(aCachePath))
         {
             return std::nullopt;
         }

        const auto cacheTime{ std::filesystem::last_write_time(aCachePath) };

        if (std::filesystem::last_write_time(aSourcePath) > cacheTime)
        {
            NL_TRACE(GCoreLogger, "shader cache stale for '{}'", aSourcePath.string());
            return std::nullopt;
        }

        auto reader{ BinaryReader::FromFile(aCachePath) };
        if (!reader.IsValid())
        {
            return std::nullopt;
        }

        // Magic
        char magic[4]{};
        reader.ReadBytes(std::span(reinterpret_cast<uint8_t*>(magic), 4));
        if (magic[0] != MAGIC[0] || magic[1] != MAGIC[1] || magic[2] != MAGIC[2] || magic[3] != MAGIC[3])
        {
            NL_WARN(GCoreLogger, "invalid shader cache magic '{}'", aCachePath.string());
            return std::nullopt;
        }

        // Version
        const uint32_t version{ reader.Read<uint32_t>() };
        if (version != VERSION)
        {
            NL_WARN(GCoreLogger, "shader cache version mismatch '{}' — will recompile", aCachePath.string());
            return std::nullopt;
        }

        RHI::Shader shader;

        // Debug name
        const uint32_t nameLen{ reader.Read<uint32_t>() };
        shader.debugName.resize(nameLen);
        reader.ReadBytes(std::span(reinterpret_cast<uint8_t*>(shader.debugName.data()), nameLen));

        // Includes - check staleness and restore
        const uint32_t includeCount{ reader.Read<uint32_t>() };
        shader.includes.reserve(includeCount);
        for (uint32_t i{ 0 }; i < includeCount; ++i)
        {
            const uint32_t len{ reader.Read<uint32_t>() };
            std::string path(len, '\0');
            reader.ReadBytes(std::span(reinterpret_cast<uint8_t*>(path.data()), len));

            const std::filesystem::path includePath{ path };
            shader.includes.push_back(includePath);

            if (std::filesystem::exists(includePath) && std::filesystem::last_write_time(includePath) > cacheTime)
            {
                NL_TRACE(GCoreLogger, "shader cache stale due to include '{}'", path);
                return std::nullopt;
            }
        }

        // Stages
        const uint32_t stageCount{ reader.Read<uint32_t>() };
        shader.stages.reserve(stageCount);
        for (uint32_t i{ 0 }; i < stageCount; ++i)
        {
            RHI::CompiledStage stage;
            stage.stage = static_cast<RHI::ShaderStage>(reader.Read<uint8_t>());

            const uint32_t entryLen{ reader.Read<uint32_t>() };
            std::string entryPoint(entryLen, '\0');
            reader.ReadBytes(std::span(reinterpret_cast<uint8_t*>(entryPoint.data()), entryLen));

            const uint64_t bytecodeSize{ reader.Read<uint64_t>() };
            stage.bytecode.resize(bytecodeSize);
            reader.ReadBytes(std::span(reinterpret_cast<uint8_t*>(stage.bytecode.data()), bytecodeSize));

            shader.stages.push_back(std::move(stage));
        }

        if (!shader.IsValid())
        {
            return std::nullopt;
        }

        NL_TRACE(GCoreLogger, "loaded shader cache '{}'", aCachePath.string());
        return shader;
    }

    void ShaderBlobStore::Save(const std::filesystem::path& aCachePath, const RHI::Shader& aShader, const RHI::ShaderDesc& aDesc)
    {
        std::filesystem::create_directories(aCachePath.parent_path());

        BinaryWriter writer;

        writer.WriteBytes(std::span(reinterpret_cast<const uint8_t*>(MAGIC), 4));
        writer.Write(VERSION);

        // Debug name
        writer.Write(static_cast<uint32_t>(aShader.debugName.size()));
        writer.WriteBytes(std::span(reinterpret_cast<const uint8_t*>(aShader.debugName.data()), aShader.debugName.size()));

        // Includes
        writer.Write(static_cast<uint32_t>(aShader.includes.size()));
        for (const auto& include : aShader.includes)
        {
            const std::string path{ include.string() };
            writer.Write(static_cast<uint32_t>(path.size()));
            writer.WriteBytes(std::span(reinterpret_cast<const uint8_t*>(path.data()), path.size()));
        }

        // Stages
        writer.Write(static_cast<uint32_t>(aShader.stages.size()));
        for (uint32_t i{ 0 }; i < aShader.stages.size(); ++i)
        {
            const auto& stage{ aShader.stages[i] };
            writer.Write(static_cast<uint8_t>(stage.stage));

            std::string entryPoint;
            for (const auto& stageDesc : aDesc.stages)
            {
                if (stageDesc.stage == stage.stage)
                {
                    entryPoint = stageDesc.entryPoint;
                    break;
                }
            }
            writer.Write(static_cast<uint32_t>(entryPoint.size()));
            writer.WriteBytes(std::span(reinterpret_cast<const uint8_t*>(entryPoint.data()), entryPoint.size()));

            writer.Write(static_cast<uint64_t>(stage.bytecode.size()));
            writer.WriteBytes(std::span(reinterpret_cast<const uint8_t*>(stage.bytecode.data()), stage.bytecode.size()));
        }

        if (writer.SaveToFile(aCachePath))
        {
            NL_TRACE(GCoreLogger, "saved shader cache '{}'", aCachePath.string());
        }
        else
        {
            NL_WARN(GCoreLogger, "failed to save shader cache '{}'", aCachePath.string());
        }
    }
}
