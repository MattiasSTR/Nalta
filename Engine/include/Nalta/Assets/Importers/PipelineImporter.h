#pragma once
#include "Nalta/Assets/Importers/IAssetImporter.h"

namespace Nalta::Graphics
{
    class ShaderCompiler;
}

namespace Nalta
{
    class PipelineImporter final : public IAssetImporter
    {
    public:
        explicit PipelineImporter(Graphics::ShaderCompiler* aShaderCompiler);

        [[nodiscard]] bool CanImport(const std::string& aExtension) const override;
        [[nodiscard]] std::unique_ptr<RawAssetData> Import(const AssetPath& aPath) const override;
        [[nodiscard]] std::string GetName() const override { return "PipelineImporter"; }

    private:
        Graphics::ShaderCompiler* myShaderCompiler{ nullptr };
    };
}