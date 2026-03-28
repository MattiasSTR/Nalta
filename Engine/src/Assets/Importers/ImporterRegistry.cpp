#include "npch.h"
#include "Nalta/Assets/Importers/ImporterRegistry.h"
#include "Nalta/Assets/Importers/IAssetImporter.h"
#include "Nalta/Assets/AssetPath.h"

namespace Nalta
{
    void ImporterRegistry::Register(std::unique_ptr<IAssetImporter> aImporter)
    {
        NL_SCOPE_CORE("ImporterRegistry");
        NL_INFO(GCoreLogger, "registered importer '{}'", aImporter->GetName());
        myImporters.push_back(std::move(aImporter));
    }

    IAssetImporter* ImporterRegistry::GetImporter(const AssetPath& aPath) const
    {
        return GetImporterForExtension(aPath.GetExtension());
    }

    IAssetImporter* ImporterRegistry::GetImporterForExtension(const std::string& aExtension) const
    {
        for (const auto& importer : myImporters)
        {
            if (importer->CanImport(aExtension))
            {
                return importer.get();
            }
        }
        return nullptr;
    }

    bool ImporterRegistry::CanImport(const AssetPath& aPath) const
    {
        return GetImporter(aPath) != nullptr;
    }
}