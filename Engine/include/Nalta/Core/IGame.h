#pragma once

namespace Nalta
{
    struct InitContext;
    struct UpdateContext;
    struct SceneViewContext;

    class IGame
    {
    public:
        virtual ~IGame() = default;

        virtual void Initialize(const InitContext& aContext) = 0;
        virtual void Shutdown() = 0;
        virtual void Update(const UpdateContext& aContext) = 0;
        virtual void BuildSceneView(SceneViewContext& aContext) = 0;
    };
}