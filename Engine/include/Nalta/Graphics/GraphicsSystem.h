#pragma once
#include "ConstantBufferDesc.h"
#include "ConstantBufferHandle.h"
#include "IndexBufferDesc.h"
#include "IndexBufferHandle.h"
#include "PipelineDesc.h"
#include "PipelineHandle.h"
#include "RenderSurfaceDesc.h"
#include "RenderSurfaceHandle.h"
#include "VertexBufferDesc.h"
#include "VertexBufferHandle.h"
#include "Nalta/Platform/WindowHandle.h"

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
        
        [[nodiscard]] Graphics::PipelineHandle CreatePipeline(const Graphics::PipelineDesc& aDesc);
        void DestroyPipeline(Graphics::PipelineHandle aHandle);
        
        [[nodiscard]] Graphics::VertexBufferHandle CreateVertexBuffer(const Graphics::VertexBufferDesc& aDesc, std::span<const std::byte> aData);
        void DestroyVertexBuffer(Graphics::VertexBufferHandle aHandle);
        
        [[nodiscard]] Graphics::IndexBufferHandle CreateIndexBuffer(const Graphics::IndexBufferDesc& aDesc, std::span<const std::byte> aData);
        void DestroyIndexBuffer(Graphics::IndexBufferHandle aHandle);
        
        [[nodiscard]] Graphics::ConstantBufferHandle CreateConstantBuffer(const Graphics::ConstantBufferDesc& aDesc);
        void DestroyConstantBuffer(Graphics::ConstantBufferHandle aHandle);

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
        
        struct SurfaceEntry
        {
            WindowHandle window;
            std::unique_ptr<Graphics::IRenderSurface> surface;
        };
        std::vector<SurfaceEntry> mySurfaces;
    };
}
