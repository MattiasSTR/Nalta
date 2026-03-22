#pragma once
#include <Nalta/Core/IGame.h>
#include <Nalta/Graphics/PipelineHandle.h>
#include <Nalta/Graphics/VertexBufferHandle.h>

class SandboxGame final : public Nalta::IGame
{
public:
    void Initialize(const Nalta::InitContext& aContext) override;
    void Shutdown()                                     override;
    void Update(const Nalta::UpdateContext& aContext)   override;
    void BuildRenderFrame(Nalta::RenderFrameContext& aContext) override;

private:
    Nalta::Graphics::PipelineHandle myTrianglePipeline;
    Nalta::Graphics::VertexBufferHandle myTriangleVB;
};
