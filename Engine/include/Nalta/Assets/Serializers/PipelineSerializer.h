#pragma once

namespace Nalta
{
    struct RawPipelineData; 
    class BinaryWriter; 
    class BinaryReader;
    
    class PipelineSerializer
    {
    public:
        static void Write(const RawPipelineData& aRawData, BinaryWriter& aWriter);
        [[nodiscard]] static RawPipelineData Read(BinaryReader& aReader);
    };
}