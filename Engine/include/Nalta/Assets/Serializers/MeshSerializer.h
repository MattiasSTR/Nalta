#pragma once

namespace Nalta
{
    struct RawMeshData; 
    class BinaryWriter; 
    class BinaryReader;
    
    class MeshSerializer
    {
    public:
        static void Write(const RawMeshData& aRawData, BinaryWriter& aWriter);
        [[nodiscard]] static RawMeshData Read(BinaryReader& aReader);
    };
}