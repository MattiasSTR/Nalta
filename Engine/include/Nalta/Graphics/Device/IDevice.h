#pragma once
#include "DeviceDesc.h"
#include "Nalta/Graphics/Buffers/ConstantBufferDesc.h"
#include "Nalta/Graphics/Buffers/IndexBufferDesc.h"
#include "Nalta/Graphics/Buffers/VertexBufferDesc.h"
#include "Nalta/Graphics/Commands/IRenderContext.h"
#include "Nalta/Graphics/Pipeline/IPipeline.h"
#include "Nalta/Graphics/Pipeline/PipelineDesc.h"
#include "Nalta/Graphics/Surface/IRenderSurface.h"
#include "Nalta/Graphics/Surface/RenderSurfaceDesc.h"

#include <memory>
#include <span>

namespace Nalta
{
    class IWindow;
    
    namespace Graphics
    {
        class IDevice
        {
        public:
            virtual ~IDevice() = default;
            
            virtual void Initialize(const DeviceDesc& aDesc) = 0;
            
            // Shutdown device and release all GPU resources
            virtual void Shutdown() = 0;
            
            // Create GPU resources
            [[nodiscard]] virtual std::unique_ptr<IVertexBuffer> CreateVertexBuffer(const VertexBufferDesc& aDesc, std::span<const std::byte> aData) = 0;
            [[nodiscard]] virtual std::unique_ptr<IIndexBuffer> CreateIndexBuffer(const IndexBufferDesc& aDesc, std::span<const std::byte> aData) = 0;
            [[nodiscard]] virtual std::unique_ptr<IConstantBuffer> CreateConstantBuffer(const ConstantBufferDesc& aDesc) = 0;
            
            [[nodiscard]] virtual std::unique_ptr<IRenderSurface> CreateRenderSurface(const RenderSurfaceDesc& aDesc) = 0;
            [[nodiscard]] virtual std::unique_ptr<IPipeline> CreatePipeline(const PipelineDesc& aDesc) = 0;
            
            [[nodiscard]] virtual std::unique_ptr<IRenderContext> CreateRenderContext() = 0;
            
            [[nodiscard]] virtual uint32_t GetFrameIndex() const = 0;
            [[nodiscard]] virtual uint32_t GetFramesInFlight()  const = 0;
            
            virtual void BeginFrame() const = 0;
            virtual void EndFrame()   const = 0;
            
            virtual void FlushUploads() = 0;
            
            virtual void Signal() const = 0;
            virtual void WaitForGPU() const = 0;
            virtual void SignalAndWait() const = 0;
        };
    }
}
