#pragma once
#include "Nalta/Graphics/IDevice.h"

#include <memory>

struct IDXGIFactory7;
struct ID3D12Device10;
struct ID3D12CommandQueue;
struct ID3D12GraphicsCommandList7;

namespace Nalta::Graphics
{
    class IRenderSurface;
    
    class DX12Device final : public IDevice
    {
    public:
        DX12Device();
        ~DX12Device() override;

        void Initialize(const DeviceDesc& aDesc) override;
        void Shutdown() override;
        
        void BeginFrame() const override;
        void EndFrame()   const override;
        
        std::unique_ptr<Buffer> CreateBuffer(const BufferDesc& aDesc) override;
        
        std::unique_ptr<IRenderSurface> CreateRenderSurface(const RenderSurfaceDesc& aDesc) override;
        
        std::shared_ptr<ComputeContext> CreateComputeContext() override;
        std::shared_ptr<GraphicsContext> CreateGraphicsContext() override;
        std::shared_ptr<UploadContext> CreateUploadContext() override;
        
        void Signal()        const override;
        void WaitForGPU()    const override;
        void SignalAndWait() const override;
        
        [[nodiscard]] IDXGIFactory7* GetFactory() const;
        [[nodiscard]] ID3D12Device10* GetDevice() const;
        [[nodiscard]] ID3D12CommandQueue* GetCommandQueue() const;
        [[nodiscard]] ID3D12GraphicsCommandList7* GetCommandList()  const;
        [[nodiscard]] uint64_t GetFenceValue()     const;
        [[nodiscard]] uint64_t GetCompletedValue() const;
        
    private:
        void InitDebugLayer() const;
        void InitInfoQueue() const;
        void SelectAdapter() const;
        void CreateCommandQueue() const;
        void CreateCommandAllocators() const;
        void CreateCommandList() const;
        void CreateFence() const;

        struct Impl;
        std::unique_ptr<Impl> myImpl;
    };
}
