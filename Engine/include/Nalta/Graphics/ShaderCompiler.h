#pragma once
#include "Shader.h"
#include "ShaderDesc.h"

#include <filesystem>
#include <memory>
#include <unordered_map>

namespace Nalta::Graphics
{
    class ShaderCompiler
    {
    public:
        ShaderCompiler();
        ~ShaderCompiler();

        void Initialize();
        void Shutdown();

        // Returns a compiled Shader with bytecode for all requested stages
        // Returns nullptr if any stage fails to compile
        [[nodiscard]] std::shared_ptr<Shader> Compile(const ShaderDesc& aDesc);
        [[nodiscard]] std::shared_ptr<Shader> Recompile(const ShaderDesc& aDesc);

        void InvalidateCache(const std::filesystem::path& aPath);

    private:
        [[nodiscard]] std::shared_ptr<Shader> CompileInternal(const ShaderDesc& aDesc) const;
        [[nodiscard]] bool CompileStage(const ShaderStageDesc& aStage, const ShaderDesc& aDesc, Shader& aOutShader) const;
        [[nodiscard]] static std::string BuildCacheKey(const ShaderDesc& aDesc);

        struct Impl;
        std::unique_ptr<Impl> myImpl;

        std::unordered_map<std::string, std::shared_ptr<Shader>> myCache;
    };
}