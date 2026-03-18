#pragma once
#include "Nalta/Graphics/Device.h"

namespace Nalta::Graphics
{
    class RenderSurface;
    
    class DX12Device : public Device
    {
    public:
        DX12Device();
        ~DX12Device() override;

        void Initialize() override;
        void Shutdown() override;
        
        std::unique_ptr<Buffer> CreateBuffer(const BufferDesc& aDesc) override;
        
        std::unique_ptr<RenderSurface> CreateRenderSurface(const RenderSurfaceDesc& aDesc) override;
        
        std::shared_ptr<ComputeContext> CreateComputeContext() override;
        std::shared_ptr<GraphicsContext> CreateGraphicsContext() override;
        std::shared_ptr<UploadContext> CreateUploadContext() override;
        
        void Present(RenderSurface* aSurface) override;
        
    private:
        void InitDebugLayer() const;
        void InitInfoQueue() const;
        void SelectAdapter() const;
        void CreateCommandQueue() const;

        struct Impl;
        std::unique_ptr<Impl> myImpl;
    };
}
