#include "npch.h"
#include "Nalta/Graphics/GraphicsSystem.h"

#include "Nalta/Graphics/GraphicsFactory.h"
#include "Nalta/Graphics/RenderSurfaceDesc.h"

namespace Nalta
{
    using namespace Graphics;

    GraphicsSystem::GraphicsSystem() = default;
    GraphicsSystem::~GraphicsSystem() = default;

    void GraphicsSystem::Initialize()
    {
        N_ASSERT(myDevice == nullptr, "GraphicsSystem: already initialized");

        myDevice = CreateDevice();
        myDevice->Initialize();

        NL_INFO(GCoreLogger, "GraphicsSystem: initialized");
    }

    void GraphicsSystem::Shutdown()
    {
        mySurfaces.clear();

        if (myDevice)
        {
            myDevice->Shutdown();
            myDevice.reset();
        }
        
        NL_INFO(GCoreLogger, "GraphicsSystem: shutdown");
    }

    RenderSurfaceHandle GraphicsSystem::CreateSurface(const RenderSurfaceDesc& aDesc)
    {
        N_ASSERT(aDesc.window.IsValid(), "GraphicsSystem: invalid window handle");

        auto surface{ myDevice->CreateRenderSurface(aDesc) };
        const RenderSurfaceHandle handle{ surface.get() };
        mySurfaces.push_back({ aDesc.window, std::move(surface) });

        NL_INFO(GCoreLogger, "GraphicsSystem: surface created");
        return handle;
    }

    void GraphicsSystem::DestroySurface(const RenderSurfaceHandle aHandle)
    {
        std::erase_if(mySurfaces, [&](const SurfaceEntry& aEntry)
        {
            return aEntry.surface.get() == aHandle.Get();
        });

        NL_INFO(GCoreLogger, "GraphicsSystem: surface destroyed");
    }
    
    void GraphicsSystem::DestroySurface(const WindowHandle aWindow)
    {
        std::erase_if(mySurfaces, [&](const SurfaceEntry& aEntry)
        {
            return aEntry.window == aWindow;
        });

        NL_INFO(GCoreLogger, "GraphicsSystem: surface destroyed for window");
    }

    void GraphicsSystem::Present(const RenderSurfaceHandle aHandle) const
    {
        N_ASSERT(aHandle.IsValid(), "GraphicsSystem: invalid surface handle");
        myDevice->Present(aHandle.Get());
    }
}
