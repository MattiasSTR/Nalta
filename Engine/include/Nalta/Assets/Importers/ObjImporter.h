#pragma once
#include "Nalta/Assets/Importers/IAssetImporter.h"

namespace Nalta
{
    class ObjImporter final : public IAssetImporter
    {
    public:
        [[nodiscard]] bool CanImport(const std::string& aExtension) const override;
        [[nodiscard]] std::unique_ptr<RawMeshData> Import(const AssetPath& aPath) const override;
        [[nodiscard]] std::string GetName() const override { return "ObjImporter"; }
    };
}