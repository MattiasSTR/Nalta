#pragma once
#include "Nalta/Graphics/Buffers/IIndexBuffer.h"
#include "Nalta/Graphics/DX12/IDX12GPUResource.h"

#include <memory>

struct ID3D12Resource;

namespace Nalta::Graphics
{
    class DX12IndexBuffer final : public IIndexBuffer, public IDX12GPUResource
    {
    public:
        DX12IndexBuffer(uint32_t aCount, IndexFormat aFormat);
        ~DX12IndexBuffer() override;

        // IBuffer
        [[nodiscard]] uint32_t GetSizeInBytes() const override;
        [[nodiscard]] bool IsValid() const override;

        // IIndexBuffer
        [[nodiscard]] uint32_t GetIndexCount() const override;
        [[nodiscard]] IndexFormat GetFormat() const override;

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

        uint32_t myCount{ 0 };
        IndexFormat myFormat{ IndexFormat::Uint32 };
    };
}