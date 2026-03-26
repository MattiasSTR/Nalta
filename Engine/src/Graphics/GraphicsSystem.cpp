#include "npch.h"
#include "Nalta/Graphics/GraphicsSystem.h"

#include "Nalta/Graphics/GraphicsFactory.h"
#include "Nalta/Graphics/Buffers/IIndexBuffer.h"
#include "Nalta/Graphics/Buffers/IVertexBuffer.h"
#include "Nalta/Graphics/Buffers/IConstantBuffer.h"
#include "Nalta/Graphics/Surface/IRenderSurface.h"
#include "Nalta/Graphics/RenderResources/IDepthBuffer.h"
#include "Nalta/Graphics/Shader/ShaderCompiler.h"
#include "Nalta/Platform/IWindow.h"

namespace Nalta
{
    using namespace Graphics;

    GraphicsSystem::GraphicsSystem() = default;
    GraphicsSystem::~GraphicsSystem() = default;

    void GraphicsSystem::Initialize()
    {
        NL_SCOPE_CORE("GraphicsSystem");
        N_CORE_ASSERT(myDevice == nullptr, "already initialized");

        myDevice = CreateDevice();
        
        DeviceDesc deviceDesc;
        deviceDesc.framesInFlight = 2;

        myDevice->Initialize(deviceDesc);
        myRenderContext = myDevice->CreateRenderContext();
        
        myShaderCompiler = std::make_unique<ShaderCompiler>();
        myShaderCompiler->Initialize();

        NL_INFO(GCoreLogger, "initialized");
    }

    void GraphicsSystem::Shutdown()
    {
        NL_SCOPE_CORE("GraphicsSystem");
        myShaderCompiler->Shutdown();
        
        if (myDevice)
        {
            myDevice->SignalAndWait(); // Drain GPU before releasing any surfaces
        }
        
        myDepthBuffers.clear();
        myConstantBuffers.clear();
        myVertexBuffers.clear();
        myIndexBuffers.clear();
        myPipelines.clear();
        mySurfaces.clear();

        if (myDevice)
        {
            myDevice->Shutdown();
            myDevice.reset();
        }
        
        NL_INFO(GCoreLogger, "shutdown");
    }

    void GraphicsSystem::BeginFrame() const
    {
        myDevice->BeginFrame();

        for (const auto& entry : mySurfaces)
        {
            const uint32_t width{ entry.window->GetWidth() };
            const uint32_t height{ entry.window->GetHeight() };

            if (width != entry.surface->GetWidth() || height != entry.surface->GetHeight())
            {
                myDevice->SignalAndWait();
                entry.surface->Resize(width, height);
                
                if (entry.depthBuffer.IsValid())
                {
                    entry.depthBuffer->Resize(width, height);
                }
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
        NL_SCOPE_CORE("GraphicsSystem");
        myDevice->FlushUploads();
        NL_TRACE(GCoreLogger, "uploads flushed");
    }

    RenderSurfaceHandle GraphicsSystem::CreateSurface(const RenderSurfaceDesc& aDesc)
    {
        NL_SCOPE_CORE("GraphicsSystem");
        N_CORE_ASSERT(aDesc.window.IsValid(), "invalid window handle");

        auto surface{ myDevice->CreateRenderSurface(aDesc) };
        const RenderSurfaceHandle handle{ surface.get() };
        mySurfaces.push_back({ aDesc.window, std::move(surface) });

        NL_INFO(GCoreLogger, "surface created");
        return handle;
    }

    void GraphicsSystem::DestroySurface(const RenderSurfaceHandle aHandle)
    {
        NL_SCOPE_CORE("GraphicsSystem");
        
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

        NL_INFO(GCoreLogger, "surface destroyed");
    }
    
    void GraphicsSystem::DestroySurface(const WindowHandle aWindow)
    {
        NL_SCOPE_CORE("GraphicsSystem");
        
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

        NL_INFO(GCoreLogger, "surface destroyed for window");
    }

    void GraphicsSystem::SetSurfaceDepthBuffer(RenderSurfaceHandle aSurface, DepthBufferHandle aDepthBuffer)
    {
        const auto it{ std::ranges::find_if(mySurfaces, [&](const SurfaceEntry& aEntry)
        {
            return aEntry.surface.get() == aSurface.Get();
        }) };

        if (it != mySurfaces.end())
        {
            it->depthBuffer = aDepthBuffer;
        }
    }

    PipelineHandle GraphicsSystem::CreatePipeline(const PipelineDesc& aDesc)
    {
        NL_SCOPE_CORE("GraphicsSystem");
        
        auto pipeline{ myDevice->CreatePipeline(aDesc) };
        if (pipeline == nullptr)
        {
            NL_ERROR(GCoreLogger, "failed to create pipeline");
            return PipelineHandle{};
        }

        const PipelineHandle handle{ pipeline.get() };
        myPipelines.push_back(std::move(pipeline));

        NL_INFO(GCoreLogger, "pipeline created");
        return handle;
    }

    void GraphicsSystem::DestroyPipeline(const PipelineHandle aHandle)
    {
        NL_SCOPE_CORE("GraphicsSystem");
        
        std::erase_if(myPipelines, [&](const std::unique_ptr<IPipeline>& p)
        {
            return p.get() == aHandle.Get();
        });

        NL_INFO(GCoreLogger, "pipeline destroyed");
    }

    VertexBufferHandle GraphicsSystem::CreateVertexBuffer(const VertexBufferDesc& aDesc, const std::span<const std::byte> aData)
    {
        NL_SCOPE_CORE("GraphicsSystem");
        
        N_CORE_ASSERT(aDesc.stride > 0, "vertex buffer stride must be > 0");
        N_CORE_ASSERT(aDesc.count  > 0, "vertex buffer count must be > 0");
        N_CORE_ASSERT(!aData.empty(),   "vertex buffer data must not be empty");

        auto buffer{ myDevice->CreateVertexBuffer(aDesc, aData) };
        
        const VertexBufferHandle handle{ buffer.get() };
        myVertexBuffers.push_back(std::move(buffer));
        
        NL_INFO(GCoreLogger, "vertex buffer created ({} vertices, {} stride)",aDesc.count, aDesc.stride);
        return handle;
    }

    void GraphicsSystem::DestroyVertexBuffer(const VertexBufferHandle aHandle)
    {
        std::erase_if(myVertexBuffers, [&](const std::unique_ptr<IVertexBuffer>& b)
        {
            return b.get() == aHandle.Get();
        });
        NL_INFO(GCoreLogger, "vertex buffer destroyed");
    }

    IndexBufferHandle GraphicsSystem::CreateIndexBuffer(const IndexBufferDesc& aDesc, const std::span<const std::byte> aData)
    {
        NL_SCOPE_CORE("GraphicsSystem");
        
        N_CORE_ASSERT(aDesc.count > 0, "index buffer count must be > 0");
        N_CORE_ASSERT(!aData.empty(), "index buffer data must not be empty");

        auto buffer{ myDevice->CreateIndexBuffer(aDesc, aData) };
        const IndexBufferHandle handle{ buffer.get() };
        myIndexBuffers.push_back(std::move(buffer));

        NL_INFO(GCoreLogger, "index buffer created ({} indices)", aDesc.count);
        return handle;
    }

    void GraphicsSystem::DestroyIndexBuffer(const IndexBufferHandle aHandle)
    {
        std::erase_if(myIndexBuffers, [&](const std::unique_ptr<IIndexBuffer>& b)
        {
            return b.get() == aHandle.Get();
        });
        NL_INFO(GCoreLogger, "index buffer destroyed");
    }

    ConstantBufferHandle GraphicsSystem::CreateConstantBuffer(const ConstantBufferDesc& aDesc)
    {
        NL_SCOPE_CORE("GraphicsSystem");
        
        N_CORE_ASSERT(aDesc.size > 0, "constant buffer size must be > 0");

        auto buffer{ myDevice->CreateConstantBuffer(aDesc) };
        const ConstantBufferHandle handle{ buffer.get() };
        myConstantBuffers.push_back(std::move(buffer));

        NL_INFO(GCoreLogger, "constant buffer created ({} bytes)", aDesc.size);
        return handle;
    }

    void GraphicsSystem::DestroyConstantBuffer(ConstantBufferHandle aHandle)
    {
        NL_SCOPE_CORE("GraphicsSystem");
        
        std::erase_if(myConstantBuffers, [&](const std::unique_ptr<IConstantBuffer>& b)
        {
            return b.get() == aHandle.Get();
        });
        NL_INFO(GCoreLogger, "constant buffer destroyed");
    }

    DepthBufferHandle GraphicsSystem::CreateDepthBuffer(const DepthBufferDesc& aDesc)
    {
        NL_SCOPE_CORE("GraphicsSystem");

        auto buffer{ myDevice->CreateDepthBuffer(aDesc) };
        const DepthBufferHandle handle{ buffer.get() };
        myDepthBuffers.push_back(std::move(buffer));

        NL_INFO(GCoreLogger, "depth buffer created ({}x{})", aDesc.width, aDesc.height);
        return handle;
    }

    void GraphicsSystem::DestroyDepthBuffer(const DepthBufferHandle aHandle)
    {
        NL_SCOPE_CORE("GraphicsSystem");

        myDevice->SignalAndWait();
        std::erase_if(myDepthBuffers, [&](const std::unique_ptr<IDepthBuffer>& b)
        {
            return b.get() == aHandle.Get();
        });

        NL_INFO(GCoreLogger, "depth buffer destroyed");
    }
}
