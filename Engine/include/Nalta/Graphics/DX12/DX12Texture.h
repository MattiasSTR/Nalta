#pragma once
#include "Nalta/Graphics/Texture/ITexture.h"
#include "Nalta/Graphics/DX12/IDX12GPUResource.h"

#include <atomic>
#include <cstdint>
#include <wrl/client.h>

struct ID3D12Resource;

namespace Nalta::Graphics
{
    class DX12Texture final : public ITexture, public IDX12GPUResource
    {
    public:
        DX12Texture(uint32_t aWidth, uint32_t aHeight, uint32_t aMipLevels, TextureFormat aFormat, uint32_t aSRVIndex);
        ~DX12Texture() override = default;

        // ITexture
        [[nodiscard]] bool IsValid() const override { return myResource != nullptr; }
        [[nodiscard]] uint32_t GetWidth() const override { return myWidth; }
        [[nodiscard]] uint32_t GetHeight() const override { return myHeight; }
        [[nodiscard]] uint32_t GetMipLevels() const override { return myMipLevels; }
        [[nodiscard]] TextureFormat GetFormat() const override { return myFormat; }
        [[nodiscard]] uint32_t GetSRVIndex() const override { return mySRVIndex; }

        // IDX12GPUResource
        void SetResource(ID3D12Resource* aResource) override;
        [[nodiscard]] bool IsReady() const override { return myReady.load(); }
        [[nodiscard]] uint64_t GetGPUAddress() const override;
        [[nodiscard]] uint32_t GetSizeInBytes() const override;

        // Takes ownership of the resource — keeps it alive for the texture lifetime
        void OwnResource(Microsoft::WRL::ComPtr<ID3D12Resource> aResource);
        void MarkReady() { myReady.store(true); }
        [[nodiscard]] ID3D12Resource* GetResource() const { return myResource; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> myOwnedResource;
        ID3D12Resource* myResource{ nullptr };
        uint32_t myWidth{ 0 };
        uint32_t myHeight{ 0 };
        uint32_t myMipLevels{ 1 };
        TextureFormat myFormat{ TextureFormat::RGBA8_UNORM };
        uint32_t mySRVIndex { 0 }; // index into device SRV heap
        std::atomic<bool> myReady{ false };
    };
}