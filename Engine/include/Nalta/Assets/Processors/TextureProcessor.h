#pragma once

namespace Nalta
{
    struct RawTextureData; 
    struct Texture; 
    class GraphicsSystem;
    
    class TextureProcessor
    {
    public:
        [[nodiscard]] static bool Process(const RawTextureData& aRawData, Texture& outTexture, GraphicsSystem& aGraphicsSystem);
    };
}