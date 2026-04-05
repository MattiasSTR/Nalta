#pragma once

namespace Nalta
{
    namespace Graphics
    {
        class GPUResourceManager;
    }
    
    struct RawMeshData; 
    struct Mesh; 
    
    class MeshProcessor
    {
    public:
        [[nodiscard]] static bool Process(const RawMeshData& aRaw, Mesh& outMesh, Graphics::GPUResourceManager& aGpuResources);

    };
}