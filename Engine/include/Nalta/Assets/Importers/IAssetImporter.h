#pragma once
#include <memory>
#include <string>

namespace Nalta
{
    struct RawAssetData;
    class AssetPath;

    class IAssetImporter
    {
    public:
        virtual ~IAssetImporter() = default;

        // Returns true if this importer can handle the given file extension
        [[nodiscard]] virtual bool CanImport(const std::string& aExtension) const = 0;

        // Import the file at the given path into raw mesh data
        // Returns nullptr on failure
        [[nodiscard]] virtual std::unique_ptr<RawAssetData> Import(const AssetPath& aPath) const = 0;

        [[nodiscard]] virtual std::string GetName() const = 0;
    };
}