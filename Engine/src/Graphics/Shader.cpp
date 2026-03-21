#include "npch.h"
#include "Nalta/Graphics/Shader.h"

namespace Nalta::Graphics
{
    void Shader::SetBytecode(const ShaderStage aStage, ShaderBytecode aBytecode)
    {
        myBytecodes[aStage] = std::move(aBytecode);
    }

    const ShaderBytecode& Shader::GetBytecode(const ShaderStage aStage) const
    {
        const auto it{ myBytecodes.find(aStage) };
        N_CORE_ASSERT(it != myBytecodes.end(), "Shader: requested stage not present");
        return it->second;
    }

    bool Shader::HasStage(const ShaderStage aStage) const
    {
        return myBytecodes.contains(aStage);
    }
}