#include "npch.h"
#include "Nalta/Graphics/SceneRenderer.h"

#include "Nalta/Assets/AssetManager.h"
#include "Nalta/Assets/Mesh.h"
#include "Nalta/Assets/Pipeline.h"
#include "Nalta/Assets/Texture.h"
#include "Nalta/Core/Math.h"
#include "Nalta/Graphics/GraphicsSystem.h"

namespace Nalta::Graphics
{
    struct TransformData
    {
        interop::float4x4 model;
        interop::float4x4 viewProjection;
    };
    
    void SceneRenderer::Initialize(GraphicsSystem* aGraphicsSystem)
    {
        myGraphicsSystem = aGraphicsSystem;

        ConstantBufferDesc cbDesc;
        cbDesc.size = sizeof(TransformData);
        myTransformCB = myGraphicsSystem->CreateConstantBuffer(cbDesc);
    }
    
    void SceneRenderer::Shutdown()
    {
        myTransformCB = {};
    }
    
    void SceneRenderer::BuildFrame(const AssetManager* aAssetManager, const SceneView& aView, RenderFrame& outFrame) const
    {
        outFrame.Clear();

        for (const auto& entry : aView.meshes)
        {
            const Mesh* mesh{ aAssetManager->GetMesh(entry.mesh) };
            const Pipeline* pipeline{ aAssetManager->GetPipeline(entry.pipeline) };
            const Texture* texture{ aAssetManager->GetTexture(entry.albedo) };
            
            // This is still here because I don't have a realiable way yet to determine root parameter indices
            if (!pipeline)
            {
                continue;
            }

            const TransformData transformData
            {
                .model = entry.transform,
                .viewProjection = aView.camera.viewProjection
            };

            outFrame.SetPipeline(pipeline->gpuHandle);
            outFrame.UpdateConstantBuffer(myTransformCB, &transformData, sizeof(transformData));
            outFrame.SetConstantBuffer(myTransformCB, RootParam_Transform);
            outFrame.SetTexture(texture->gpuHandle, RootParam_Texture);
            outFrame.SetVertexBuffer(mesh->vb);
            outFrame.SetIndexBuffer(mesh->ib);
            outFrame.DrawIndexed(mesh->GetIndexCount());
        }
    }
}