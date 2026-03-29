#pragma once
#include "Nalta/Assets/Asset.h"
#include "Nalta/Graphics/Texture/TextureHandle.h"

namespace Nalta
{
    class TextureAsset final : public Asset
    {
    public:
        explicit TextureAsset(AssetPath aPath) : Asset(std::move(aPath)) {}
        ~TextureAsset() override = default;

        [[nodiscard]] AssetType GetAssetType() const override { return AssetType::Texture; }
        [[nodiscard]] Graphics::TextureHandle GetTextureHandle() const { return myTextureHandle; }

    private:
        friend class TextureProcessor;
        
        Graphics::TextureHandle myTextureHandle;
    };
}