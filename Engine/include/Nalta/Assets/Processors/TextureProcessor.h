#pragma once

namespace Nalta
{
    namespace Graphics
    {
        class GPUResourceManager;
    }
    
    struct RawTextureData; 
    struct Texture; 
    
    class TextureProcessor
    {
    public:
        [[nodiscard]] static bool Process(const RawTextureData& aRawData, Texture& outTexture, Graphics::GPUResourceManager& aGpuResources);
    };
}