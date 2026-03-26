#pragma once
#include "Buffers/ConstantBufferDesc.h"
#include "Buffers/ConstantBufferHandle.h"
#include "Buffers/IndexBufferDesc.h"
#include "Buffers/IndexBufferHandle.h"
#include "Buffers/VertexBufferDesc.h"
#include "Buffers/VertexBufferHandle.h"
#include "Pipeline/PipelineDesc.h"
#include "Pipeline/PipelineHandle.h"
#include "Surface/RenderSurfaceDesc.h"
#include "Surface/RenderSurfaceHandle.h"
#include "Nalta/Platform/WindowHandle.h"
#include "RenderResources/DepthBufferDesc.h"
#include "RenderResources/DepthBufferHandle.h"

#include <memory>
#include <vector>
#include <span>

namespace Nalta
{
    namespace Graphics
    {
        class IDevice;
        class IRenderContext;
        class IVertexBuffer;
        class IIndexBuffer;
        class ShaderCompiler;
    }
    
    class GraphicsSystem
    {
    public:
        GraphicsSystem();
        ~GraphicsSystem();
        
        void Initialize();
        void Shutdown();
        
        void BeginFrame() const;
        void EndFrame() const;
        
        void FlushUploads() const;

        [[nodiscard]] Graphics::RenderSurfaceHandle CreateSurface(const Graphics::RenderSurfaceDesc& aDesc);
        void DestroySurface(Graphics::RenderSurfaceHandle aHandle);
        void DestroySurface(WindowHandle aWindow);
        void SetSurfaceDepthBuffer(Graphics::RenderSurfaceHandle aSurface, Graphics::DepthBufferHandle aDepthBuffer);
        
        [[nodiscard]] Graphics::PipelineHandle CreatePipeline(const Graphics::PipelineDesc& aDesc);
        void DestroyPipeline(Graphics::PipelineHandle aHandle);
        
        [[nodiscard]] Graphics::VertexBufferHandle CreateVertexBuffer(const Graphics::VertexBufferDesc& aDesc, std::span<const std::byte> aData);
        void DestroyVertexBuffer(Graphics::VertexBufferHandle aHandle);
        
        [[nodiscard]] Graphics::IndexBufferHandle CreateIndexBuffer(const Graphics::IndexBufferDesc& aDesc, std::span<const std::byte> aData);
        void DestroyIndexBuffer(Graphics::IndexBufferHandle aHandle);
        
        [[nodiscard]] Graphics::ConstantBufferHandle CreateConstantBuffer(const Graphics::ConstantBufferDesc& aDesc);
        void DestroyConstantBuffer(Graphics::ConstantBufferHandle aHandle);
        
        [[nodiscard]] Graphics::DepthBufferHandle CreateDepthBuffer(const Graphics::DepthBufferDesc& aDesc);
        void DestroyDepthBuffer(Graphics::DepthBufferHandle aHandle);

        [[nodiscard]] Graphics::IDevice* GetDevice() const { return myDevice.get(); }
        [[nodiscard]] Graphics::IRenderContext* GetRenderContext() const { return myRenderContext.get(); }
        [[nodiscard]] Graphics::ShaderCompiler* GetShaderCompiler() const { return myShaderCompiler.get(); }

    private:
        std::unique_ptr<Graphics::IDevice> myDevice;
        std::unique_ptr<Graphics::IRenderContext> myRenderContext;
        std::unique_ptr<Graphics::ShaderCompiler> myShaderCompiler;
        
        std::vector<std::unique_ptr<Graphics::IPipeline>> myPipelines;
        std::vector<std::unique_ptr<Graphics::IVertexBuffer>> myVertexBuffers;
        std::vector<std::unique_ptr<Graphics::IIndexBuffer>> myIndexBuffers;
        std::vector<std::unique_ptr<Graphics::IConstantBuffer>> myConstantBuffers;
        std::vector<std::unique_ptr<Graphics::IDepthBuffer>> myDepthBuffers;
        
        struct SurfaceEntry
        {
            WindowHandle window;
            std::unique_ptr<Graphics::IRenderSurface> surface;
            Graphics::DepthBufferHandle depthBuffer;
        };
        std::vector<SurfaceEntry> mySurfaces;
    };
}
