#pragma once

namespace Nalta
{
    struct InitContext;
    struct UpdateContext;
    struct RenderFrameContext;

    class IGame
    {
    public:
        virtual ~IGame() = default;

        virtual void Initialize(const InitContext& aContext) = 0;
        virtual void Shutdown() = 0;
        virtual void Update(const UpdateContext& aContext) = 0;
        virtual void BuildRenderFrame(RenderFrameContext& aContext) = 0;
    };
}