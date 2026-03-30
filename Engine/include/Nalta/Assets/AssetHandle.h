#pragma once
#include "Nalta/Assets/Mesh.h"
#include "Nalta/Assets/Texture.h"
#include "Nalta/Assets/Pipeline.h"

namespace Nalta
{
    template<typename T>
    struct AssetHandle
    {
        uint64_t id{ 0 };
        [[nodiscard]] bool IsValid() const { return id != 0; }
        bool operator==(const AssetHandle&) const = default;
    };

    using MeshHandle = AssetHandle<Mesh>;
    using TextureHandle = AssetHandle<Texture>;
    using PipelineHandle = AssetHandle<Pipeline>;
}