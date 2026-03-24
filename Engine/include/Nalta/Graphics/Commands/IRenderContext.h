#pragma once
#include "RenderFrame.h"

namespace Nalta::Graphics
{
    class IRenderContext
    {
    public:
        virtual ~IRenderContext() = default;

        // Called at the start of a frame — resets the command list
        virtual void Begin() = 0;

        // Called at the end of a frame — executes recorded commands
        virtual void Execute(const RenderFrame& aFrame) = 0;
    };
}