#pragma once
#include "Nalta/Graphics/Buffers/IConstantBuffer.h"
#include <memory>

struct ID3D12Device10;

namespace Nalta::Graphics
{
    class DX12Device;

    class DX12ConstantBuffer final : public IConstantBuffer
    {
    public:
        DX12ConstantBuffer(uint32_t aSize, DX12Device* aDevice);
        ~DX12ConstantBuffer() override;

        // IBuffer
        [[nodiscard]] uint32_t GetSizeInBytes() const override;
        [[nodiscard]] bool IsValid() const override;

        // Update current frame slot — called from render thread
        void Update(const void* aData, uint32_t aSize) const;

        // Get GPU address for current frame slot — for binding
        [[nodiscard]] uint64_t GetGPUAddress() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> myImpl;

        DX12Device* myDevice{ nullptr };
        uint32_t mySize{ 0 };
        uint32_t myAlignedSize{ 0 };
    };
}