#include "npch.h"
#include "Nalta/Graphics/GraphicsSystem.h"
#include "Nalta/Platform/IWindow.h"

namespace Nalta
{
    using namespace Graphics;

    GraphicsSystem::GraphicsSystem() = default;
    GraphicsSystem::~GraphicsSystem() = default;

    void GraphicsSystem::Initialize()
    {
        NL_SCOPE_CORE("GraphicsSystem");
        //N_CORE_ASSERT(myDevice == nullptr, "already initialized");

        //myDevice = CreateDevice();
        
        NL_INFO(GCoreLogger, "initialized");
    }

    void GraphicsSystem::Shutdown()
    {
        NL_SCOPE_CORE("GraphicsSystem");
        
        NL_INFO(GCoreLogger, "shutdown");
    }
}
