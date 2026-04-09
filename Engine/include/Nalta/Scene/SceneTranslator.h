#pragma once
#include "Nalta/Core/SceneSnapshot.h"
#include "Nalta/Scene/Scene.h"

namespace Nalta
{
    class SceneTranslator
    {
    public:
        static void Build(const Scene& aScene, SceneSnapshot& aSnapshot);
    };
}
