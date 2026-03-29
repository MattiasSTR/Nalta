#pragma once
#include "Nalta/Assets/Importers/IAssetImporter.h"

namespace Nalta
{
    class DDSImporter final : public IAssetImporter
    {
    public:
        [[nodiscard]] bool CanImport(const std::string& aExtension) const override;
        [[nodiscard]] std::unique_ptr<RawAssetData> Import(const AssetPath& aPath) const override;
        [[nodiscard]] std::string GetName() const override { return "DDSImporter"; }
    };
}