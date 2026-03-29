#pragma once
#include "Nalta/Graphics/Device/IDevice.h"

#include <memory>

struct IDXGIFactory7;
struct ID3D12Device10;
struct ID3D12CommandQueue;
struct ID3D12GraphicsCommandList7;
struct ID3D12DescriptorHeap;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

namespace Nalta::Graphics
{
    class IRenderSurface;
    class DX12CopyQueue;
    class DX12UploadBatch;
    
    class DX12Device final : public IDevice
    {
    public:
        DX12Device();
        ~DX12Device() override;

        void Initialize(const DeviceDesc& aDesc) override;
        void Shutdown() override;
        
        void BeginFrame() const override;
        void EndFrame() const override;
        
        void FlushUploads() override;
        
        [[nodiscard]] std::unique_ptr<IVertexBuffer> CreateVertexBuffer(const VertexBufferDesc& aDesc, std::span<const std::byte> aData) override;
        [[nodiscard]] std::unique_ptr<IIndexBuffer> CreateIndexBuffer(const IndexBufferDesc& aDesc, std::span<const std::byte> aData) override;
        [[nodiscard]] std::unique_ptr<IConstantBuffer> CreateConstantBuffer(const ConstantBufferDesc& aDesc) override;
        
        [[nodiscard]] std::unique_ptr<ITexture> CreateTexture(const TextureDesc& aDesc, std::span<const std::byte> aData) override;
        
        [[nodiscard]] std::unique_ptr<IRenderSurface> CreateRenderSurface(const RenderSurfaceDesc& aDesc) override;
        [[nodiscard]] std::unique_ptr<IPipeline> CreatePipeline(const PipelineDesc& aDesc) override;
        
        [[nodiscard]] std::unique_ptr<IDepthBuffer> CreateDepthBuffer(const DepthBufferDesc& aDesc) override;
        
        [[nodiscard]] std::unique_ptr<IRenderContext> CreateRenderContext() override;
        
        [[nodiscard]] uint32_t GetFrameIndex() const override;
        [[nodiscard]] uint32_t GetFramesInFlight() const override;

        void Signal() const override;
        void WaitForGPU() const override;
        void SignalAndWait() const override;
        
        [[nodiscard]] DX12CopyQueue& GetCopyQueue() const;
        [[nodiscard]] IDXGIFactory7* GetFactory() const;
        [[nodiscard]] ID3D12Device10* GetDevice() const;
        [[nodiscard]] ID3D12CommandQueue* GetCommandQueue() const;
        [[nodiscard]] ID3D12GraphicsCommandList7* GetCommandList() const;
        [[nodiscard]] uint64_t GetFenceValue() const;
        [[nodiscard]] uint64_t GetCompletedValue() const;
        
        [[nodiscard]] uint32_t AllocateSRVIndex() const;
        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle(uint32_t aIndex) const;
        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle(uint32_t aIndex) const;
        [[nodiscard]] ID3D12DescriptorHeap* GetSRVHeap() const;
        
    private:
        void InitDebugLayer() const;
        void InitInfoQueue() const;
        void SelectAdapter() const;
        void CreateCommandQueue() const;
        void CreateCommandAllocators() const;
        void CreateCommandList() const;
        void CreateFence() const;
        void CreateSRVHeap() const;

        struct Impl;
        std::unique_ptr<Impl> myImpl;
    };
}
