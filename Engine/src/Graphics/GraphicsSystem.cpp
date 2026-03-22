#include "npch.h"
#include "Nalta/Graphics/GraphicsSystem.h"

#include "Nalta/Graphics/GraphicsFactory.h"
#include "Nalta/Graphics/RenderSurfaceDesc.h"
#include "Nalta/Platform/IWindow.h"

namespace Nalta
{
    using namespace Graphics;

    GraphicsSystem::GraphicsSystem() = default;
    GraphicsSystem::~GraphicsSystem() = default;

    void GraphicsSystem::Initialize()
    {
        N_CORE_ASSERT(myDevice == nullptr, "GraphicsSystem: already initialized");

        myDevice = CreateDevice();
        
        DeviceDesc deviceDesc;
        deviceDesc.framesInFlight = 2;

        myDevice->Initialize(deviceDesc);
        myRenderContext = myDevice->CreateRenderContext();
        
        myShaderCompiler.Initialize();

        NL_INFO(GCoreLogger, "GraphicsSystem: initialized");
    }

    void GraphicsSystem::Shutdown()
    {
        myShaderCompiler.Shutdown();
        
        if (myDevice)
        {
            myDevice->SignalAndWait(); // Drain GPU before releasing any surfaces
        }
        
        myPipelines.clear();
        mySurfaces.clear();

        if (myDevice)
        {
            myDevice->Shutdown();
            myDevice.reset();
        }
        
        NL_INFO(GCoreLogger, "GraphicsSystem: shutdown");
    }

    void GraphicsSystem::BeginFrame() const
    {
        myDevice->BeginFrame();

        for (const auto& entry : mySurfaces)
        {
            const uint32_t width { entry.window->GetWidth() };
            const uint32_t height{ entry.window->GetHeight() };

            if (width != entry.surface->GetWidth() || height != entry.surface->GetHeight())
            {
                myDevice->SignalAndWait();
                entry.surface->Resize(width, height);
            }
        }
    }

    void GraphicsSystem::EndFrame() const
    {
        for (const auto& entry : mySurfaces)
        {
            entry.surface->EndRenderTarget(); // Transition to PRESENT before closing
        }

        myDevice->EndFrame(); // Close, submit, signal

        for (const auto& entry : mySurfaces)
        {
            entry.surface->Present(); // Present after GPU work is submitted
        }
    }

    RenderSurfaceHandle GraphicsSystem::CreateSurface(const RenderSurfaceDesc& aDesc)
    {
        N_CORE_ASSERT(aDesc.window.IsValid(), "GraphicsSystem: invalid window handle");

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

    PipelineHandle GraphicsSystem::CreatePipeline(const PipelineDesc& aDesc)
    {
        auto pipeline{ myDevice->CreatePipeline(aDesc) };
        if (pipeline == nullptr)
        {
            NL_ERROR(GCoreLogger, "GraphicsSystem: failed to create pipeline");
            return PipelineHandle{};
        }

        const PipelineHandle handle{ pipeline.get() };
        myPipelines.push_back(std::move(pipeline));

        NL_INFO(GCoreLogger, "GraphicsSystem: pipeline created");
        return handle;
    }

    void GraphicsSystem::DestroyPipeline(const PipelineHandle aHandle)
    {
        std::erase_if(myPipelines, [&](const std::unique_ptr<IPipeline>& p)
        {
            return p.get() == aHandle.Get();
        });

        NL_INFO(GCoreLogger, "GraphicsSystem: pipeline destroyed");
    }
}
