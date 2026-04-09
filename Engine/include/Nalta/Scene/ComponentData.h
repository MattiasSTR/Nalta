#pragma once
#include "Nalta/Assets/AssetKeys.h"
#include "Nalta/Core/Math.h"
#include "Nalta/Util/SlotKey.h"

namespace Nalta
{
    struct GameObjectKey : SlotKey { using SlotKey::SlotKey; };
    struct TransformKey  : SlotKey { using SlotKey::SlotKey; };
    struct StaticMeshKey : SlotKey { using SlotKey::SlotKey; };
    
    struct TransformData
    {
        float3 position{ 0.f, 0.f, 0.f };
        quaternion rotation{ quaternion::identity() };
        float3 scale{ 1.f, 1.f, 1.f };
    };
    
    struct StaticMeshData
    {
        MeshKey meshKey;
        TextureKey albedoKey; // Temp, replace with material key in future
        TransformKey transformKey;
        bool dirty{ false };
    };
}
