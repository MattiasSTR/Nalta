#pragma once
#include "Nalta/Graphics/Texture/TextureDesc.h"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

struct ID3D12Device10;

namespace Nalta::Graphics
{
    class DX12Texture;
    class DX12CopyQueue;
    class IDX12GPUResource;

    class DX12UploadBatch
    {
    public:
        DX12UploadBatch();
        ~DX12UploadBatch();

        void Initialize(ID3D12Device10* aDevice, DX12CopyQueue* aCopyQueue);
        void Shutdown();

        void QueueUpload(std::span<const std::byte> aData, IDX12GPUResource* aTarget);
        void QueueTextureUpload(std::span<const std::byte> aData, DX12Texture* aTarget, const TextureDesc& aDesc);
        void Flush();

        [[nodiscard]] bool HasPendingUploads() const;

    private:
        struct PendingUpload
        {
            std::vector<std::byte> data;
            IDX12GPUResource* target{ nullptr };
        };
        
        struct PendingTextureUpload
        {
            std::vector<std::byte> data;
            DX12Texture* target{ nullptr };
            TextureDesc desc;
        };

        struct Impl;
        std::unique_ptr<Impl> myImpl;

        std::vector<PendingUpload> myPendingUploads;
        std::vector<PendingTextureUpload> myPendingTextureUploads;
        ID3D12Device10* myDevice{ nullptr };
        DX12CopyQueue* myCopyQueue{ nullptr };
    };
}