#pragma once
#include <memory>
#include <vector>

namespace Nalta
{
    class IAssetSerializer;
    enum class AssetType : uint8_t;

    class SerializerRegistry
    {
    public:
        void Register(std::unique_ptr<IAssetSerializer> aSerializer);
        [[nodiscard]] IAssetSerializer* GetSerializer(AssetType aType) const;

    private:
        std::vector<std::unique_ptr<IAssetSerializer>> mySerializers;
    };
}