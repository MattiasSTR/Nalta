#pragma once
#include "Buffer.h"
#include "Context.h"
#include "RenderSurface.h"

#include <memory>

namespace Nalta
{
    class IWindow;
    
    namespace Graphics
    {
        class Device
        {
        public:
            virtual ~Device() = default;
        
            // Initialize device; may allocate GPU device/queues
            virtual void Initialize() = 0;
            
            // Shutdown device and release all GPU resources
            virtual void Shutdown() = 0;
            
            // Create GPU resources
            virtual std::unique_ptr<Buffer> CreateBuffer(const BufferDesc& aDesc) = 0;
            
            // Create contexts
            virtual std::shared_ptr<GraphicsContext> CreateGraphicsContext() = 0;
            virtual std::shared_ptr<ComputeContext> CreateComputeContext() = 0;
            virtual std::shared_ptr<UploadContext> CreateUploadContext() = 0;
            
            // Present the backbuffer of a surface
            virtual void Present(std::shared_ptr<RenderSurface> aSurface) = 0;
        };
    }
}
