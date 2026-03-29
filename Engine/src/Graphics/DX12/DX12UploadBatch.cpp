#include "npch.h"
#include "Nalta/Graphics/DX12/DX12UploadBatch.h"
#include "Nalta/Graphics/DX12/DX12CopyQueue.h"
#include "Nalta/Graphics/DX12/DX12Texture.h"
#include "Nalta/Graphics/DX12/IDX12GPUResource.h"
#include "Nalta/Graphics/DX12/DX12Util.h"

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Nalta::Graphics
{
    struct DX12UploadBatch::Impl
    {
        std::vector<ComPtr<ID3D12Resource>> stagingBuffers;
        std::vector<ComPtr<ID3D12Resource>> trackedResources;
    };

    DX12UploadBatch::DX12UploadBatch()
        : myImpl(std::make_unique<Impl>())
    {}

    DX12UploadBatch::~DX12UploadBatch() = default;
    
    void DX12UploadBatch::Initialize(ID3D12Device10* aDevice, DX12CopyQueue* aCopyQueue)
    {
        NL_SCOPE_CORE("DX12UploadBatch");
        N_CORE_ASSERT(aDevice, "null device");
        N_CORE_ASSERT(aCopyQueue, "null copy queue");

        myDevice = aDevice;
        myCopyQueue = aCopyQueue;

        NL_TRACE(GCoreLogger, "initialized");
    }
    
    void DX12UploadBatch::Shutdown()
    {
        NL_SCOPE_CORE("DX12UploadBatch");
        myImpl->stagingBuffers.clear();
        myPendingUploads.clear();
        myDevice    = nullptr;
        myCopyQueue = nullptr;

        NL_TRACE(GCoreLogger, "shutdown");
    }

    void DX12UploadBatch::QueueUpload(std::span<const std::byte> aData, IDX12GPUResource* aTarget)
    {
        NL_SCOPE_CORE("DX12UploadBatch");
        N_CORE_ASSERT(!aData.empty(), "empty data");
        N_CORE_ASSERT(aTarget, "null target");

        PendingUpload upload;
        upload.data.assign(aData.begin(), aData.end());
        upload.target = aTarget;
        myPendingUploads.push_back(std::move(upload));

        NL_TRACE(GCoreLogger, "queued upload ({} bytes)", aData.size());
    }

    void DX12UploadBatch::QueueTextureUpload(std::span<const std::byte> aData, DX12Texture* aTarget, const TextureDesc& aDesc)
    {
        NL_SCOPE_CORE("DX12UploadBatch");
        
        N_CORE_ASSERT(!aData.empty(), "empty texture data");
        N_CORE_ASSERT(aTarget, "null texture target");

        PendingTextureUpload upload;
        upload.data.assign(aData.begin(), aData.end());
        upload.target = aTarget;
        upload.desc   = aDesc;
        myPendingTextureUploads.push_back(std::move(upload));

        NL_TRACE(GCoreLogger, "queued texture upload ({}x{}, {} bytes)", aDesc.width, aDesc.height, aData.size());
    }

    void DX12UploadBatch::Flush()
    {
        if (myPendingUploads.empty())
        {
            return;
        }
        
        NL_SCOPE_CORE("DX12UploadBatch");
        
        myImpl->stagingBuffers.clear();
        myImpl->trackedResources.clear();

        myCopyQueue->Begin();
        auto* cmdList{ myCopyQueue->GetCommandList() };
        
        for (auto& upload : myPendingUploads)
        {
            const uint64_t sizeInBytes{ upload.data.size() };

            constexpr D3D12_HEAP_PROPERTIES defaultHeap
            {
                .Type                 = D3D12_HEAP_TYPE_DEFAULT,
                .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN
            };

            const D3D12_RESOURCE_DESC bufferDesc
            {
                .Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER,
                .Alignment        = 0,
                .Width            = sizeInBytes,
                .Height           = 1,
                .DepthOrArraySize = 1,
                .MipLevels        = 1,
                .Format           = DXGI_FORMAT_UNKNOWN,
                .SampleDesc       = { 1, 0 },
                .Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                .Flags            = D3D12_RESOURCE_FLAG_NONE
            };

            ComPtr<ID3D12Resource> defaultBuffer;
            if (FAILED(myDevice->CreateCommittedResource(
                &defaultHeap,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&defaultBuffer))))
            {
                NL_ERROR(GCoreLogger, "failed to create default buffer");
                continue;
            }

            DX12_SET_NAME(defaultBuffer.Get(), "GPU Buffer");

            constexpr D3D12_HEAP_PROPERTIES uploadHeap
            {
                .Type                 = D3D12_HEAP_TYPE_UPLOAD,
                .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN
            };

            ComPtr<ID3D12Resource> stagingBuffer;
            if (FAILED(myDevice->CreateCommittedResource(
                &uploadHeap,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&stagingBuffer))))
            {
                NL_ERROR(GCoreLogger, "failed to create staging buffer");
                continue;
            }

            DX12_SET_NAME(stagingBuffer.Get(), "Staging Buffer");

            void* mapped{ nullptr };
            constexpr D3D12_RANGE readRange{ 0, 0 };
            if (FAILED(stagingBuffer->Map(0, &readRange, &mapped)))
            {
                NL_ERROR(GCoreLogger, "failed to map staging buffer");
                continue;
            }

            memcpy(mapped, upload.data.data(), sizeInBytes);
            stagingBuffer->Unmap(0, nullptr);

            cmdList->CopyBufferRegion(
                defaultBuffer.Get(), 0,
                stagingBuffer.Get(), 0,
                sizeInBytes);

            upload.target->SetResource(defaultBuffer.Get());

            myImpl->stagingBuffers.push_back(std::move(stagingBuffer));
        }
        
        for (auto& upload : myPendingTextureUploads)
        {
            const auto dxgiFormat{ ToDXGIFormat(upload.desc.format) };
            const uint32_t width { upload.desc.width };
            const uint32_t height{ upload.desc.height };
            const uint32_t mips  { upload.desc.mipLevels };
        
            // Get layout for all mips to know upload buffer size
            std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(mips);
            std::vector<UINT>   numRows(mips);
            std::vector<UINT64> rowSizes(mips);
            UINT64 totalBytes{ 0 };
        
            const D3D12_RESOURCE_DESC texDesc
            {
                .Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                .Width            = width,
                .Height           = height,
                .DepthOrArraySize = 1,
                .MipLevels        = static_cast<UINT16>(mips),
                .Format           = dxgiFormat,
                .SampleDesc       = { 1, 0 },
                .Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags            = D3D12_RESOURCE_FLAG_NONE
            };
        
            myDevice->GetCopyableFootprints(&texDesc, 0, mips, 0, layouts.data(), numRows.data(), rowSizes.data(), &totalBytes);
        
            // Create staging buffer sized for all mips
            constexpr D3D12_HEAP_PROPERTIES uploadHeap
            {
                .Type                 = D3D12_HEAP_TYPE_UPLOAD,
                .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN
            };
        
            const D3D12_RESOURCE_DESC stagingDesc
            {
                .Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER,
                .Width            = totalBytes,
                .Height           = 1,
                .DepthOrArraySize = 1,
                .MipLevels        = 1,
                .Format           = DXGI_FORMAT_UNKNOWN,
                .SampleDesc       = { 1, 0 },
                .Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                .Flags            = D3D12_RESOURCE_FLAG_NONE
            };
        
            ComPtr<ID3D12Resource> stagingBuffer;
            if (FAILED(myDevice->CreateCommittedResource(
                &uploadHeap, D3D12_HEAP_FLAG_NONE, &stagingDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                IID_PPV_ARGS(&stagingBuffer))))
            {
                NL_ERROR(GCoreLogger, "failed to create texture staging buffer");
                continue;
            }
        
            DX12_SET_NAME(stagingBuffer.Get(), "Texture Staging Buffer");
        
            // Map and copy each mip
            uint8_t* mapped{ nullptr };
            constexpr D3D12_RANGE readRange{ 0, 0 };
            if (FAILED(stagingBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mapped))))
            {
                NL_ERROR(GCoreLogger, "failed to map texture staging buffer");
                continue;
            }
        
            const uint8_t* srcData{ reinterpret_cast<const uint8_t*>(upload.data.data()) };
            uint64_t srcOffset{ 0 };
        
            for (uint32_t mip{ 0 }; mip < mips; ++mip)
            {
                const auto& layout{ layouts[mip] };
                const uint64_t dstRowPitch{ layout.Footprint.RowPitch };
                const uint64_t srcRowPitch{ rowSizes[mip] };
                uint8_t* dst{ mapped + layout.Offset };
        
                for (uint32_t row{ 0 }; row < numRows[mip]; ++row)
                {
                    memcpy(dst + row * dstRowPitch, srcData + srcOffset + row * srcRowPitch, srcRowPitch);
                }
        
                srcOffset += srcRowPitch * numRows[mip];
            }
        
            stagingBuffer->Unmap(0, nullptr);
        
            // Get the texture's GPU resource - it was created in DX12Device::CreateTexture
            // We need to get it via DX12Texture but it only exposes GPU address
            // Add GetResource() to DX12Texture for this purpose
            ID3D12Resource* texResource{ upload.target->GetResource() };
            if (!texResource)
            {
                NL_ERROR(GCoreLogger, "texture has no GPU resource");
                continue;
            }
        
            // Copy each mip from staging to texture
            for (uint32_t mip{ 0 }; mip < mips; ++mip)
            {
                const D3D12_TEXTURE_COPY_LOCATION dst
                {
                    .pResource        = texResource,
                    .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                    .SubresourceIndex = mip
                };
        
                const D3D12_TEXTURE_COPY_LOCATION src
                {
                    .pResource       = stagingBuffer.Get(),
                    .Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                    .PlacedFootprint = layouts[mip]
                };
        
                cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
            }
        
            upload.target->MarkReady();
            myImpl->stagingBuffers.push_back(std::move(stagingBuffer));
        }

        myCopyQueue->End();
        myCopyQueue->ExecuteAndWait();

        myPendingUploads.clear();
        NL_TRACE(GCoreLogger, "flushed");
    }

    bool DX12UploadBatch::HasPendingUploads() const
    {
        return !myPendingUploads.empty();
    }
}
