#include "npch.h"
#include "Nalta/Assets/Serializers/SerializerRegistry.h"
#include "Nalta/Assets/Serializers/IAssetSerializer.h"
#include "Nalta/Assets/AssetState.h"

namespace Nalta
{
    void SerializerRegistry::Register(std::unique_ptr<IAssetSerializer> aSerializer)
    {
        NL_SCOPE_CORE("SerializerRegistry");
        NL_TRACE(GCoreLogger, "registered serializer for type {}", static_cast<uint8_t>(aSerializer->GetAssetType()));
        mySerializers.push_back(std::move(aSerializer));
    }

    IAssetSerializer* SerializerRegistry::GetSerializer(const AssetType aType) const
    {
        for (const auto& serializer : mySerializers)
        {
            if (serializer->GetAssetType() == aType)
            {
                return serializer.get();
            }
        }
        return nullptr;
    }
}