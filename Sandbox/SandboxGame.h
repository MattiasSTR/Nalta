#pragma once
#include <Nalta/Assets/AssetRequest.h>
#include <Nalta/Core/IGame.h>
#include <Nalta/Graphics/Buffers/ConstantBufferHandle.h>
#include <Nalta/Graphics/Buffers/IndexBufferHandle.h>
#include <Nalta/Graphics/Pipeline/PipelineHandle.h>
#include <Nalta/Graphics/Buffers/VertexBufferHandle.h>
#include <Nalta/Core/Math.h>

class SandboxGame final : public Nalta::IGame
{
public:
    void Initialize(const Nalta::InitContext& aContext) override;
    void Shutdown()                                     override;
    void Update(const Nalta::UpdateContext& aContext)   override;
    void BuildRenderFrame(Nalta::RenderFrameContext& aContext) override;

private:
    Nalta::Graphics::PipelineHandle myMeshPipeline;
    Nalta::Graphics::ConstantBufferHandle myTransformCB;
    Nalta::AssetRequest myMeshRequest;

    float3 myPosition{ 0.0f, 0.0f, -3.0f };
    float1  myYaw    { 0.0f };
    float1  myPitch  { 0.0f };
    float  myTime   { 0.0f };
};
