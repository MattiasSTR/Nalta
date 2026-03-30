#pragma once

namespace Nalta
{
    struct RawMeshData; 
    struct Mesh; 
    class GraphicsSystem;
    
    class MeshProcessor
    {
    public:
        [[nodiscard]] static bool Process(const RawMeshData& aRawData, Mesh& outMesh, GraphicsSystem& aGraphicsSystem);
    };
}