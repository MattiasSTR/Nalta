#pragma once
#include "Nalta/Assets/AssetType.h"

#include <memory>

namespace Nalta
{
    struct RawAssetData;
    class BinaryWriter;
    class BinaryReader;

    class IAssetSerializer
    {
    public:
        virtual ~IAssetSerializer() = default;

        virtual void Write(const RawAssetData& aData, BinaryWriter& aWriter) const = 0;

        [[nodiscard]] virtual std::unique_ptr<RawAssetData> Read(BinaryReader& aReader) const = 0;

        [[nodiscard]] virtual AssetType GetAssetType() const = 0;
    };
}
