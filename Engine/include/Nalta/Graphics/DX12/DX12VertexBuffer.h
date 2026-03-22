#pragma once
#include "Nalta/Graphics/IVertexBuffer.h"
#include "Nalta/Graphics/DX12/IDX12GPUResource.h"
#include <memory>

struct ID3D12Resource;

namespace Nalta::Graphics
{
    class DX12VertexBuffer final : public IVertexBuffer, public IDX12GPUResource
    {
    public:
        DX12VertexBuffer(uint32_t aStride, uint32_t aCount);
        ~DX12VertexBuffer() override;

        // IBuffer
        [[nodiscard]] uint32_t GetSizeInBytes() const override;
        [[nodiscard]] bool IsValid() const override;

        // IVertexBuffer
        [[nodiscard]] uint32_t GetStride() const override;
        [[nodiscard]] uint32_t GetVertexCount() const override;

        // IGPUResource
        [[nodiscard]] bool IsReady() const override;
        [[nodiscard]] uint64_t GetGPUAddress() const override;

        // IDX12GPUResource
        void SetResource(ID3D12Resource* aResource) override;

        // DX12 specific
        [[nodiscard]] ID3D12Resource* GetResource() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> myImpl;

        uint32_t myStride{ 0 };
        uint32_t myCount{ 0 };
    };
}