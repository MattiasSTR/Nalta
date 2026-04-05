#include "npch.h"
#include "Nalta/RHI/D3D12/Contexts/D3D12UploadContext.h"
#include "Nalta/RHI/D3D12/D3D12Device.h"

namespace Nalta::RHI::D3D12
{
    UploadContext::UploadContext(Device& aDevice, const uint64_t aUploadHeapSize)
        : Context(aDevice, D3D12_COMMAND_LIST_TYPE_COPY)
        , myUploadHeapSize(aUploadHeapSize)
    {
        BufferCreationDesc heapDesc{};
        heapDesc.size      = aUploadHeapSize;
        heapDesc.access    = BufferAccessFlags::CpuToGpu;
        heapDesc.debugName = "Upload Heap";

        myUploadHeap = aDevice.CreateBuffer(heapDesc);

        N_CORE_ASSERT(myUploadHeap != nullptr, "Failed to create upload heap buffer");
        N_CORE_ASSERT(myUploadHeap->mappedData != nullptr, "Upload heap is not mapped");

        NL_TRACE(GCoreLogger, "upload context initialized [heap: {} MB]", aUploadHeapSize / (1024ull * 1024ull));
    }
    
    UploadContext::~UploadContext()
    {
        N_CORE_ASSERT(myPendingTextureUploads.empty(), "UploadContext destroyed with pending texture uploads");
        N_CORE_ASSERT(myTextureUploadsInProgress.empty(), "UploadContext destroyed with texture uploads in progress");
        N_CORE_ASSERT(myPendingBufferUploads.empty(), "UploadContext destroyed with pending buffer uploads");
        N_CORE_ASSERT(myBufferUploadsInProgress.empty(), "UploadContext destroyed with buffer uploads in progress");

        if (myUploadHeap && myUploadHeap->mappedData)
        {
            myUploadHeap->resource->Unmap(0, nullptr);
            myUploadHeap->mappedData = nullptr;
        }

        if (myUploadHeap)
        {
            SafeRelease(myUploadHeap->allocation);
            SafeRelease(myUploadHeap->resource);
        }
    }
    
    void UploadContext::AddTextureUpload(TextureResource* aTexture, TextureUploadDesc aUploadDesc)
    {
        N_CORE_ASSERT(aTexture != nullptr, "Cannot upload to null texture");
        N_CORE_ASSERT(!aUploadDesc.mips.empty(), "No mip data provided for upload");

        const TextureCreationDesc& desc{ aUploadDesc.desc };
        const uint32_t numSubs{ static_cast<uint32_t>(desc.mipLevels * desc.arraySize) };

        N_CORE_ASSERT(aUploadDesc.mips.size() == numSubs, "Mip count does not match desc — expected mipLevels * arraySize entries");

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width              = desc.width;
        resourceDesc.Height             = desc.height;
        resourceDesc.DepthOrArraySize   = desc.depth > 1 ? static_cast<UINT16>(desc.depth) : static_cast<UINT16>(desc.arraySize);
        resourceDesc.MipLevels          = desc.mipLevels;
        resourceDesc.Format             = IsDepthFormat(desc.format) ? DepthFormatToTypeless(desc.format) : TextureFormatToDXGI(desc.format);
        resourceDesc.SampleDesc.Count   = 1;
        resourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Dimension          = desc.depth > 1 ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        PendingTextureUpload pending{};
        pending.texture    = aTexture;
        pending.uploadDesc = std::move(aUploadDesc);
        pending.layouts.resize(numSubs);
        pending.numRows.resize(numSubs);
        pending.rowSizes.resize(numSubs);

        myDevice.GetD3D12Device()->GetCopyableFootprints(
            &resourceDesc,
            0,
            numSubs,
            0,
            pending.layouts.data(),
            pending.numRows.data(),
            pending.rowSizes.data(),
            &pending.totalSize);

        myPendingTextureUploads.push_back(std::move(pending));
    }

    void UploadContext::AddBufferUpload(BufferResource* aBuffer, const BufferUploadDesc aUploadDesc)
    {
        N_CORE_ASSERT(aBuffer != nullptr, "Cannot upload to null buffer");
        N_CORE_ASSERT(!aUploadDesc.data.empty(), "Buffer upload data is empty");
        N_CORE_ASSERT(aUploadDesc.data.size_bytes() <= myUploadHeapSize, "Buffer upload exceeds heap size");

        PendingBufferUpload pending{};
        pending.buffer = aBuffer;
        pending.data.resize(aUploadDesc.data.size_bytes());
        memcpy(pending.data.data(), aUploadDesc.data.data(), aUploadDesc.data.size_bytes());

        myPendingBufferUploads.push_back(std::move(pending));
    }

    void UploadContext::ProcessUploads()
    {
        if (myPendingTextureUploads.empty() && myPendingBufferUploads.empty())
        {
            return;
        }

        Reset(); // opens the command list (closes previous if open)
        
        uint32_t numBuffersProcessed{ 0 };

        for (auto& pending : myPendingBufferUploads)
        {
            const uint64_t dataSize{ static_cast<uint64_t>(pending.data.size()) };
            const uint64_t alignedSize{ (dataSize + 255) & ~255ull };

            if (myCurrentOffset + alignedSize > myUploadHeapSize)
            {
                NL_WARN(GCoreLogger, "Upload heap full - {} buffer upload(s) deferred to next frame", myPendingBufferUploads.size() - numBuffersProcessed);
                break;
            }

            memcpy(myUploadHeap->mappedData + myCurrentOffset, pending.data.data(), dataSize);

            myCommandList->CopyBufferRegion(
                pending.buffer->resource,
                0,
                myUploadHeap->resource,
                myCurrentOffset,
                dataSize);

            myCurrentOffset += alignedSize;
            myBufferUploadsInProgress.push_back(pending.buffer);
            numBuffersProcessed++;
        }

        if (numBuffersProcessed > 0)
        {
            myPendingBufferUploads.erase(myPendingBufferUploads.begin(), myPendingBufferUploads.begin() + numBuffersProcessed);
        }
        
        uint32_t numTexturesProcessed{ 0 };
        
        for (auto& pending : myPendingTextureUploads)
        {
            if (myCurrentOffset + pending.totalSize > myUploadHeapSize)
            {
                NL_WARN(GCoreLogger, "Upload heap full - {} upload(s) deferred to next frame", myPendingTextureUploads.size() - numTexturesProcessed);
                break;
            }
        
            const uint32_t numSubs{ static_cast<uint32_t>(pending.layouts.size()) };
            const uint64_t heapOffset{ myCurrentOffset };
        
            for (uint32_t i{ 0 }; i < numSubs; ++i)
            {
                const TextureMipData& mip{ pending.uploadDesc.mips[i] };
                const auto& layout{ pending.layouts[i] };
                const uint64_t rowPitch{ layout.Footprint.RowPitch };
        
                uint8_t* dst{ myUploadHeap->mappedData + heapOffset + layout.Offset };
                const std::byte* src{ mip.data.data() };
        
                for (uint32_t row{ 0 }; row < pending.numRows[i]; ++row)
                {
                    memcpy(dst, src, pending.rowSizes[i]);
                    dst += rowPitch;
                    src += mip.rowPitch;
                }
            }
        
            for (uint32_t i{ 0 }; i < numSubs; ++i)
            {
                D3D12_TEXTURE_COPY_LOCATION dst{};
                dst.pResource        = pending.texture->resource;
                dst.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                dst.SubresourceIndex = i;
        
                D3D12_TEXTURE_COPY_LOCATION src{};
                src.pResource               = myUploadHeap->resource;
                src.Type                    = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                src.PlacedFootprint         = pending.layouts[i];
                src.PlacedFootprint.Offset += heapOffset;
        
                myCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
            }
        
            myCurrentOffset += pending.totalSize;
            myCurrentOffset  = (myCurrentOffset + 511) & ~511ull;
        
            myTextureUploadsInProgress.push_back(pending.texture);
            numTexturesProcessed++;
        }
        
        if (numTexturesProcessed > 0)
        {
            myPendingTextureUploads.erase(myPendingTextureUploads.begin(), myPendingTextureUploads.begin() + numTexturesProcessed);
        }

        Close();
    }
    
    void UploadContext::ResolveUploads()
    {
        for (auto* texture : myTextureUploadsInProgress)
        {
            texture->state   = D3D12_RESOURCE_STATE_COMMON; // promoted during copy, back to common
            texture->isReady = true;
        }
        
        for (auto* buffer : myBufferUploadsInProgress)
        {
            buffer->state   = D3D12_RESOURCE_STATE_COMMON;
            buffer->isReady = true;
        }

        myTextureUploadsInProgress.clear();
        myBufferUploadsInProgress.clear();
        myCurrentOffset = 0;
    }
}