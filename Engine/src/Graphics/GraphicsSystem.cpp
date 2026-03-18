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
        
        DeviceDesc deviceDesc;
        deviceDesc.framesInFlight = 2;

        myDevice->Initialize(deviceDesc);

        NL_INFO(GCoreLogger, "GraphicsSystem: initialized");
    }

    void GraphicsSystem::Shutdown()
    {
        if (myDevice)
        {
            myDevice->SignalAndWait(); // Drain GPU before releasing any surfaces
        }
        
        mySurfaces.clear();

        if (myDevice)
        {
            myDevice->Shutdown();
            myDevice.reset();
        }
        
        NL_INFO(GCoreLogger, "GraphicsSystem: shutdown");
    }

    void GraphicsSystem::BeginFrame(const float aClearColor[4]) const
    {
        myDevice->BeginFrame();

        for (const auto& entry : mySurfaces)
        {
            entry.surface->Clear(aClearColor);
        }
    }

    void GraphicsSystem::EndFrame() const
    {
        myDevice->EndFrame();

        for (const auto& entry : mySurfaces)
        {
            entry.surface->Present();
        }
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
        const auto it{ std::ranges::find_if(mySurfaces, [&](const SurfaceEntry& aEntry)
        {
            return aEntry.surface.get() == aHandle.Get();
        }) };

        if (it == mySurfaces.end())
        {
            return;
        }

        myDevice->SignalAndWait();
        mySurfaces.erase(it);

        NL_INFO(GCoreLogger, "GraphicsSystem: surface destroyed");
    }
    
    void GraphicsSystem::DestroySurface(const WindowHandle aWindow)
    {
        const auto it{ std::ranges::find_if(mySurfaces, [&](const SurfaceEntry& aEntry)
        {
            return aEntry.window == aWindow;
        }) };

        if (it == mySurfaces.end())
        {
            return;
        }

        myDevice->SignalAndWait();
        mySurfaces.erase(it);

        NL_INFO(GCoreLogger, "GraphicsSystem: surface destroyed for window");
    }
}
