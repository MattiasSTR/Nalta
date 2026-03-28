#pragma once
#include <memory>
#include <string>
#include <vector>

namespace Nalta
{
    class IAssetImporter;
    class AssetPath;

    class ImporterRegistry
    {
    public:
        ImporterRegistry() = default;
        ~ImporterRegistry() = default;

        void Register(std::unique_ptr<IAssetImporter> aImporter);

        // Find importer for the given asset path based on extension
        [[nodiscard]] IAssetImporter* GetImporter(const AssetPath& aPath) const;
        [[nodiscard]] IAssetImporter* GetImporterForExtension(const std::string& aExtension) const;

        [[nodiscard]] bool CanImport(const AssetPath& aPath) const;

    private:
        std::vector<std::unique_ptr<IAssetImporter>> myImporters;
    };
}
