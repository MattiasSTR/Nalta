#pragma once
#include "Nalta/Graphics/Device.h"

#include <memory>

struct IDXGIFactory7;
struct ID3D12Device10;
struct ID3D12CommandQueue;

namespace Nalta::Graphics
{
    class RenderSurface;
    
    class DX12Device final : public Device
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
        
        [[nodiscard]] IDXGIFactory7* GetFactory()      const;
        [[nodiscard]] ID3D12Device10* GetDevice()       const;
        [[nodiscard]] ID3D12CommandQueue* GetCommandQueue() const;
        
    private:
        void InitDebugLayer() const;
        void InitInfoQueue() const;
        void SelectAdapter() const;
        void CreateCommandQueue() const;

        struct Impl;
        std::unique_ptr<Impl> myImpl;
    };
}
