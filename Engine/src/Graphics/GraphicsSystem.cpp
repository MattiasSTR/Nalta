#include "npch.h"
#include "Nalta/Graphics/GraphicsSystem.h"

#include "Nalta/Graphics/GraphicsFactory.h"
#include "Nalta/Graphics/IIndexBuffer.h"
#include "Nalta/Graphics/IVertexBuffer.h"
#include "Nalta/Graphics/RenderSurfaceDesc.h"
#include "Nalta/Graphics/ShaderCompiler.h"
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
        
        myShaderCompiler = std::make_unique<ShaderCompiler>();
        myShaderCompiler->Initialize();

        NL_INFO(GCoreLogger, "GraphicsSystem: initialized");
    }

    void GraphicsSystem::Shutdown()
    {
        myShaderCompiler->Shutdown();
        
        if (myDevice)
        {
            myDevice->SignalAndWait(); // Drain GPU before releasing any surfaces
        }
        
        myVertexBuffers.clear();
        myIndexBuffers.clear();
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

    void GraphicsSystem::FlushUploads() const
    {
        myDevice->FlushUploads();
        NL_TRACE(GCoreLogger, "GraphicsSystem: uploads flushed");
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

    VertexBufferHandle GraphicsSystem::CreateVertexBuffer(const VertexBufferDesc& aDesc, const std::span<const std::byte> aData)
    {
        N_CORE_ASSERT(aDesc.stride > 0, "GraphicsSystem: vertex buffer stride must be > 0");
        N_CORE_ASSERT(aDesc.count  > 0, "GraphicsSystem: vertex buffer count must be > 0");
        N_CORE_ASSERT(!aData.empty(),   "GraphicsSystem: vertex buffer data must not be empty");

        auto buffer{ myDevice->CreateVertexBuffer(aDesc, aData) };
        
        const VertexBufferHandle handle{ buffer.get() };
        myVertexBuffers.push_back(std::move(buffer));
        
        NL_INFO(GCoreLogger, "GraphicsSystem: vertex buffer created ({} vertices, {} stride)",aDesc.count, aDesc.stride);
        return handle;
    }

    void GraphicsSystem::DestroyVertexBuffer(const VertexBufferHandle aHandle)
    {
        std::erase_if(myVertexBuffers, [&](const std::unique_ptr<IVertexBuffer>& b)
        {
            return b.get() == aHandle.Get();
        });
        NL_INFO(GCoreLogger, "GraphicsSystem: vertex buffer destroyed");
    }

    IndexBufferHandle GraphicsSystem::CreateIndexBuffer(const IndexBufferDesc& aDesc, const std::span<const std::byte> aData)
    {
        N_CORE_ASSERT(aDesc.count > 0, "GraphicsSystem: index buffer count must be > 0");
        N_CORE_ASSERT(!aData.empty(), "GraphicsSystem: index buffer data must not be empty");

        auto buffer{ myDevice->CreateIndexBuffer(aDesc, aData) };
        const IndexBufferHandle handle{ buffer.get() };
        myIndexBuffers.push_back(std::move(buffer));

        NL_INFO(GCoreLogger, "GraphicsSystem: index buffer created ({} indices)", aDesc.count);
        return handle;
    }

    void GraphicsSystem::DestroyIndexBuffer(const IndexBufferHandle aHandle)
    {
        std::erase_if(myIndexBuffers, [&](const std::unique_ptr<IIndexBuffer>& b)
        {
            return b.get() == aHandle.Get();
        });
        NL_INFO(GCoreLogger, "GraphicsSystem: index buffer destroyed");
    }
}
