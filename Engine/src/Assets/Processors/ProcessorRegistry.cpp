#include "npch.h"
#include "Nalta/Assets/Processors/ProcessorRegistry.h"
#include "Nalta/Assets/Processors/IAssetProcessor.h"

namespace Nalta
{
    void ProcessorRegistry::Register(std::unique_ptr<IAssetProcessor> aProcessor)
    {
        NL_SCOPE_CORE("ProcessorRegistry");
        NL_TRACE(GCoreLogger, "registered processor for type {}", static_cast<uint8_t>(aProcessor->GetAssetType()));
        myProcessors.push_back(std::move(aProcessor));
    }

    IAssetProcessor* ProcessorRegistry::GetProcessor(const AssetType aType) const
    {
        for (const auto& processor : myProcessors)
        {
            if (processor->GetAssetType() == aType)
            {
                return processor.get();
            }
        }
        return nullptr;
    }
}