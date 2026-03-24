#pragma once
#include <cstdint>
#include <vector>
#include <unordered_map>
#include "ShaderStage.h"

namespace Nalta::Graphics
{
    struct ShaderBytecode
    {
        std::vector<uint8_t> code;
        std::vector<uint8_t> reflection;

        [[nodiscard]] const void* GetData() const { return code.data(); }
        [[nodiscard]] size_t GetSize() const { return code.size(); }
        [[nodiscard]] const void* GetReflectionData() const { return reflection.data(); }
        [[nodiscard]] size_t GetReflectionSize() const { return reflection.size(); }
        [[nodiscard]] bool IsValid() const { return !code.empty(); }
        [[nodiscard]] bool HasReflection() const { return !reflection.empty(); }
    };

    class Shader
    {
    public:
        void SetBytecode(ShaderStage aStage, ShaderBytecode aBytecode);

        [[nodiscard]] const ShaderBytecode& GetBytecode(ShaderStage aStage) const;
        [[nodiscard]] bool HasStage(ShaderStage aStage) const;

    private:
        std::unordered_map<ShaderStage, ShaderBytecode> myBytecodes;
    };
}