#pragma once
#include "Nalta/Assets/Serializers/IAssetSerializer.h"

namespace Nalta
{
    class MeshSerializer final : public IAssetSerializer
    {
    public:
        void Write(const RawAssetData& aData, BinaryWriter& aWriter) const override;

        [[nodiscard]] std::unique_ptr<RawAssetData> Read(BinaryReader& aReader) const override;

        [[nodiscard]] AssetType GetAssetType() const override { return AssetType::Mesh; }
    };
}