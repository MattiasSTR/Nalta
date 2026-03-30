#pragma once

namespace Nalta
{
    struct RawTextureData; 
    class BinaryWriter; 
    class BinaryReader;
    
    class TextureSerializer
    {
    public:
        static void Write(const RawTextureData& aRawData, BinaryWriter& aWriter);
        [[nodiscard]] static RawTextureData Read(BinaryReader& aReader);
    };
}