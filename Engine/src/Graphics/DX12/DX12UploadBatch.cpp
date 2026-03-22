#include "npch.h"
#include "Nalta/Graphics/DX12/DX12UploadBatch.h"
#include "Nalta/Graphics/DX12/DX12CopyQueue.h"
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
    };

    DX12UploadBatch::DX12UploadBatch()
        : myImpl(std::make_unique<Impl>())
    {}

    DX12UploadBatch::~DX12UploadBatch() = default;
    
    void DX12UploadBatch::Initialize(ID3D12Device10* aDevice, DX12CopyQueue* aCopyQueue)
    {
        N_CORE_ASSERT(aDevice, "DX12UploadBatch: null device");
        N_CORE_ASSERT(aCopyQueue, "DX12UploadBatch: null copy queue");

        myDevice = aDevice;
        myCopyQueue = aCopyQueue;

        NL_TRACE(GCoreLogger, "DX12UploadBatch: initialized");
    }
    
    void DX12UploadBatch::Shutdown()
    {
        myImpl->stagingBuffers.clear();
        myPendingUploads.clear();
        myDevice    = nullptr;
        myCopyQueue = nullptr;

        NL_TRACE(GCoreLogger, "DX12UploadBatch: shutdown");
    }

    void DX12UploadBatch::QueueUpload(std::span<const std::byte> aData, IDX12GPUResource* aTarget)
    {
        N_CORE_ASSERT(!aData.empty(), "DX12UploadBatch: empty data");
        N_CORE_ASSERT(aTarget, "DX12UploadBatch: null target");

        PendingUpload upload;
        upload.data.assign(aData.begin(), aData.end());
        upload.target = aTarget;
        myPendingUploads.push_back(std::move(upload));

        NL_TRACE(GCoreLogger, "DX12UploadBatch: queued upload ({} bytes)", aData.size());
    }

    void DX12UploadBatch::Flush()
    {
        if (myPendingUploads.empty()) return;
        
        myImpl->stagingBuffers.clear();

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
                NL_ERROR(GCoreLogger, "DX12UploadBatch: failed to create default buffer");
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
                NL_ERROR(GCoreLogger, "DX12UploadBatch: failed to create staging buffer");
                continue;
            }

            DX12_SET_NAME(stagingBuffer.Get(), "Staging Buffer");

            void* mapped{ nullptr };
            constexpr D3D12_RANGE readRange{ 0, 0 };
            if (FAILED(stagingBuffer->Map(0, &readRange, &mapped)))
            {
                NL_ERROR(GCoreLogger, "DX12UploadBatch: failed to map staging buffer");
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

        myCopyQueue->End();
        myCopyQueue->ExecuteAndWait();

        myPendingUploads.clear();
        NL_TRACE(GCoreLogger, "DX12UploadBatch: flushed");
    }

    bool DX12UploadBatch::HasPendingUploads() const
    {
        return !myPendingUploads.empty();
    }
}
