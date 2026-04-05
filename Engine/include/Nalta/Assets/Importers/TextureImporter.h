#pragma once
#include "Nalta/Assets/Importers/IAssetImporter.h"
#include "Nalta/RHI/Types/RHIEnums.h"

#include <string>

namespace Nalta
{

    class TextureImporter final : public IAssetImporter
    {
    public:
        explicit TextureImporter(std::string aExtension);

        [[nodiscard]] bool CanImport(const std::string& aExtension) const override;
        [[nodiscard]] std::unique_ptr<RawAssetData> Import(const AssetPath& aPath) const override;
        [[nodiscard]] std::string GetName() const override;
        
        // Called by TextureDescriptorImporter with explicit settings
        [[nodiscard]] std::unique_ptr<RawAssetData> ImportWithSettings(const AssetPath& aPath, RHI::TextureFormat aTargetFormat, bool aGenerateMips) const;

    private:
        std::string myExtension;
    };
}