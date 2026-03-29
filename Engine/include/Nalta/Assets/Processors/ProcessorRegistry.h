#pragma once
#include <memory>
#include <vector>
#include <cstdint>

namespace Nalta
{
    class IAssetProcessor;
    enum class AssetType : uint8_t;

    class ProcessorRegistry
    {
    public:
        void Register(std::unique_ptr<IAssetProcessor> aProcessor);
        [[nodiscard]] IAssetProcessor* GetProcessor(AssetType aType) const;

    private:
        std::vector<std::unique_ptr<IAssetProcessor>> myProcessors;
    };
}