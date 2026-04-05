#pragma once
#include "RHIEnums.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Nalta::RHI
{
    struct ShaderStageDesc;

    struct CompiledStage
    {
        std::vector<uint8_t> bytecode{};
        ShaderStage stage{};

        [[nodiscard]] bool IsValid() const { return !bytecode.empty(); }
        [[nodiscard]] const void* GetData() const { return bytecode.data(); }
        [[nodiscard]] size_t GetSize() const { return bytecode.size(); }
    };
    
    struct Shader
    {
        std::vector<CompiledStage> stages{};
        std::string debugName{};

        [[nodiscard]] const CompiledStage* GetStage(const ShaderStage aStage) const
        {
            for (const auto& s : stages)
            {
                if (s.stage == aStage)
                {
                    return &s;
                }
            }
            return nullptr;
        }

        [[nodiscard]] bool HasStage(const ShaderStage aStage) const { return GetStage(aStage) != nullptr; }
        [[nodiscard]] bool IsValid() const { return !stages.empty(); }
    };
    
    struct ShaderDesc
    {
        std::filesystem::path filePath{};  // filesystem include was missing from RHITypes.h
        std::vector<ShaderStageDesc> stages{};
        std::unordered_map<std::string, std::string> defines{};
        std::string debugName{};
    };
}
