#pragma once
#include <memory>

namespace Nalta
{
    struct RawMeshData;
    class MeshAsset;
    class GraphicsSystem;

    class MeshProcessor
    {
    public:
        MeshProcessor() = default;
        explicit MeshProcessor(GraphicsSystem* aGraphicsSystem);

        // Fills in the MeshAsset GPU resources from RawMeshData
        // Returns false on failure
        bool Process(const RawMeshData& aRawData, MeshAsset& aOutMesh) const;

    private:
        GraphicsSystem* myGraphicsSystem{ nullptr };
    };
}