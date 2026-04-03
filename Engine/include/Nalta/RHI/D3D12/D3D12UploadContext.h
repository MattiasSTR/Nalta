#pragma once
#include "D3D12Context.h"
#include "Nalta/RHI/RHI.h"
#include "Nalta/RHI/RHITypes.h"
#include "Nalta/RHI/D3D12/D3D12Common.h"
#include "Nalta/RHI/D3D12/D3D12Texture.h"
#include "Nalta/RHI/D3D12/D3D12Buffer.h"

namespace Nalta::RHI::D3D12
{
    class Device;

    struct PendingTextureUpload
    {
        TextureResource* texture{ nullptr };
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts{};
        std::vector<uint32_t> numRows{};
        std::vector<uint64_t> rowSizes{};
        uint64_t totalSize{ 0 };
        TextureUploadDesc uploadDesc{};
    };

    class UploadContext final : public Context
    {
    public:
        UploadContext(Device& aDevice, uint64_t aUploadHeapSize);
        ~UploadContext() override;

        UploadContext(const UploadContext&)            = delete;
        UploadContext& operator=(const UploadContext&) = delete;
        UploadContext(UploadContext&&)                 = delete;
        UploadContext& operator=(UploadContext&&)      = delete;

        void AddTextureUpload(TextureResource* aTexture, TextureUploadDesc aUploadDesc);

        // Called in PrePresent - copies data, records and closes command list
        void ProcessUploads();

        // Called in BeginFrame after fence wait - marks textures ready
        void ResolveUploads();

        [[nodiscard]] bool HasPendingWork() const { return !myUploadsInProgress.empty(); }

    private:
        std::unique_ptr<BufferResource> myUploadHeap;
        uint64_t myUploadHeapSize{ 0 };
        uint64_t myCurrentOffset{ 0 };

        std::vector<PendingTextureUpload> myPendingUploads;
        std::vector<TextureResource*> myUploadsInProgress;
    };
}