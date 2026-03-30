#pragma once

namespace Nalta
{
    struct RawPipelineData; 
    struct Pipeline; 
    class GraphicsSystem;
    
    class PipelineProcessor
    {
    public:
        [[nodiscard]] static bool Process(const RawPipelineData& aRawData, Pipeline& outPipeline, GraphicsSystem& aGraphicsSystem);
    };
}