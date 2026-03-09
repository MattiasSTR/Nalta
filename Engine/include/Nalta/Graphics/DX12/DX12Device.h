#pragma once
#include "DX12RenderSurface.h"
#include "Nalta/Graphics/Device.h"

namespace Nalta::Graphics
{
    class DX12Device : public Device
    {
    public:
        DX12Device() = default;
        ~DX12Device() override = default;
        
        void Initialize() override;
        void Shutdown() override;
        
        std::unique_ptr<Buffer> CreateBuffer(const BufferDesc& aDesc) override;
        
        std::shared_ptr<ComputeContext> CreateComputeContext() override;
        std::shared_ptr<GraphicsContext> CreateGraphicsContext() override;
        std::shared_ptr<UploadContext> CreateUploadContext() override;
        
        void Present(std::shared_ptr<RenderSurface> aSurface) override;
    };
}
