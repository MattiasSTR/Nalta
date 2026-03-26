#pragma once
#include "Nalta/Graphics/RenderResources/DepthBufferDesc.h"
#include "Nalta/Graphics/RenderResources/IDepthBuffer.h"
#include <memory>

struct ID3D12Resource;
struct ID3D12DescriptorHeap;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

namespace Nalta::Graphics
{
    class DX12Device;

    class DX12DepthBuffer final : public IDepthBuffer
    {
    public:
        DX12DepthBuffer(const DepthBufferDesc& aDesc, DX12Device* aDevice);
        ~DX12DepthBuffer() override;

        void Resize(uint32_t aWidth, uint32_t aHeight) override;

        [[nodiscard]] uint32_t GetWidth() const override;
        [[nodiscard]] uint32_t GetHeight() const override;
        [[nodiscard]] bool IsValid() const override;
        
        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;

    private:
        void CreateResources() const;
        void ReleaseResources() const;

        struct Impl;
        std::unique_ptr<Impl> myImpl;

        DX12Device* myDevice{ nullptr };
        uint32_t myWidth{ 0 };
        uint32_t myHeight{ 0 };
    };
}