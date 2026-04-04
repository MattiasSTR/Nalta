#include "npch.h"
#include "Nalta/RHI/D3D12/D3D12Device.h"

namespace Nalta::RHI::D3D12
{
    namespace
    {
        constexpr D3D_FEATURE_LEVEL MIN_FEATURE_LEVEL{ D3D_FEATURE_LEVEL_12_0 };

        constexpr D3D_FEATURE_LEVEL FEATURE_LEVELS[]
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_2,
        };
        
        const char* FeatureLevelToString(const D3D_FEATURE_LEVEL aLevel)
        {
            switch (aLevel)
            {
                case D3D_FEATURE_LEVEL_11_0: return "11_0";
                case D3D_FEATURE_LEVEL_11_1: return "11_1";
                case D3D_FEATURE_LEVEL_12_0: return "12_0";
                case D3D_FEATURE_LEVEL_12_1: return "12_1";
                case D3D_FEATURE_LEVEL_12_2: return "12_2";
                default: return "Unknown";
            }
        }
    }
    
    Device::Device()
    {
        NL_SCOPE_CORE("DX12Device::Constructor");
        InitDebugLayer();

        UINT factoryFlags{ 0 };
#ifndef N_SHIPPING
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
        if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&myFactory))))
        {
            NL_FATAL(GCoreLogger, "failed to create DXGI factory");
        }
        
        SelectAdapter();
        
        if (FAILED(D3D12CreateDevice(myAdapter, MIN_FEATURE_LEVEL, IID_PPV_ARGS(&myDevice))))
        {
            NL_FATAL(GCoreLogger, "failed to create D3D12 device");
        }
        
        D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelInfo{};
        featureLevelInfo.NumFeatureLevels = static_cast<UINT>(std::size(FEATURE_LEVELS));
        featureLevelInfo.pFeatureLevelsRequested = FEATURE_LEVELS;
        NL_DX_VERIFY(myDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelInfo, sizeof(featureLevelInfo)));
        NL_INFO(GCoreLogger, "max feature level {}", FeatureLevelToString(featureLevelInfo.MaxSupportedFeatureLevel));
        
        for (size_t i{ 0 }; i < static_cast<size_t>(QueueType::Count); ++i)
        {
            myQueues[i] = std::make_unique<Queue>(*this, static_cast<QueueType>(i));
        }
        
        N_D3D12_SET_NAME(myDevice, "Main D3D12 Device");
        
        InitAllocator();
        InitDescriptorHeaps();
        for (uint32_t i{ 0 }; i < FRAMES_IN_FLIGHT; ++i)
        {
            myUploadContexts[i] = std::make_unique<UploadContext>(*this, 64ull * 1024ull * 1024ull);
        }
        NL_TRACE(GCoreLogger, "upload contexts initialized");
        
        CheckFeatureSupport();
        InitInfoQueue();
        
        if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&myDxcUtils))))
        {
            NL_FATAL(GCoreLogger, "failed to create DXC utils for reflection");
        }

        NL_INFO(GCoreLogger, "initialized");
    }

    Device::~Device()
    {
        NL_SCOPE_CORE("DX12Device::Destructor");
        
        for (auto& queue : myQueues)
        {
            if (queue)
            {
                queue->WaitForIdle();
            }
        }
        
#ifndef N_SHIPPING
        ID3D12DebugDevice* debugDevice;
        myDevice->QueryInterface(IID_PPV_ARGS(&debugDevice));
#endif
        
        for (uint32_t i{ 0 }; i < FRAMES_IN_FLIGHT; ++i)
        {
            ProcessDestructions(i);
        }
        
        for (auto& ctx : myUploadContexts)
        {
            ctx.reset();
        }
        
        // Explicitly destroy queues before device
        for (auto& queue : myQueues)
        {
            queue.reset();
        }
        
        myBindlessHeap.reset();
        mySamplerHeap.reset();
        myRTVHeap.reset();
        myDSVHeap.reset();
        
        SafeRelease(myAllocator);
        SafeRelease(myDxcUtils);
        SafeRelease(myAdapter);
        SafeRelease(myFactory);
        SafeRelease(myDevice);
        
#ifndef N_SHIPPING
        if (debugDevice != nullptr)
        {
            debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
            SafeRelease(debugDevice);
        }
#endif
        
        NL_INFO(GCoreLogger, "shutdown");
    }

    void Device::BeginFrame()
    {
        myFrameIndex = (myFrameIndex + 1) % FRAMES_IN_FLIGHT;

        // Wait on the fences from FramesInFlight frames ago before reusing this slot
        const FrameFences& fences{ myFrameFences[myFrameIndex] };
        GetQueue(QueueType::Graphics).WaitForFenceCPUBlocking(fences.graphicsFence);
        GetQueue(QueueType::Compute).WaitForFenceCPUBlocking(fences.computeFence);
        GetQueue(QueueType::Copy).WaitForFenceCPUBlocking(fences.copyFence);
        
        myUploadContexts[myFrameIndex]->ResolveUploads();
        myUploadContexts[myFrameIndex]->Reset();

        ProcessDestructions(myFrameIndex);
        myContextSubmissions[myFrameIndex].clear();
    }

    void Device::PrePresent()
    {
        myUploadContexts[myFrameIndex]->ProcessUploads();

        if (myUploadContexts[myFrameIndex]->HasPendingWork())
        {
            ID3D12CommandList* lists[]{ myUploadContexts[myFrameIndex]->GetCommandList() };
            myFrameFences[myFrameIndex].copyFence = GetQueue(QueueType::Copy).ExecuteCommandLists(lists);
        }

        myFrameFences[myFrameIndex].computeFence = GetQueue(QueueType::Compute).SignalFence();
    }

    void Device::EndFrame()
    {
        myFrameFences[myFrameIndex].graphicsFence = GetQueue(QueueType::Graphics).SignalFence();
    }

    ContextSubmissionResult Device::SubmitContextWork(Context& aContext)
    {
        aContext.FlushBarriers();

        ID3D12CommandList* lists[]{ aContext.GetCommandList() };
        uint64_t fenceValue{ 0 };

        switch (aContext.GetType())
        {
            case D3D12_COMMAND_LIST_TYPE_DIRECT:
                fenceValue = GetQueue(QueueType::Graphics).ExecuteCommandLists(lists);
                break;
            case D3D12_COMMAND_LIST_TYPE_COMPUTE:
                fenceValue = GetQueue(QueueType::Compute).ExecuteCommandLists(lists);
                break;
            case D3D12_COMMAND_LIST_TYPE_COPY:
                fenceValue = GetQueue(QueueType::Copy).ExecuteCommandLists(lists);
                break;
            default:
                N_CORE_ASSERT(false, "Unknown context type");
                break;
        }

        ContextSubmissionResult result{};
        result.frameId         = myFrameIndex;
        result.submissionIndex = static_cast<uint32_t>(myContextSubmissions[myFrameIndex].size());

        myContextSubmissions[myFrameIndex].push_back({ fenceValue, aContext.GetType() });

        return result;
    }

    void Device::WaitOnContextWork(const ContextSubmissionResult aSubmission, const ContextWaitType aWaitType)
    {
        const auto& submission{ myContextSubmissions[aSubmission.frameId][aSubmission.submissionIndex] };
    
        Queue* sourceQueue{ nullptr };
        switch (submission.type)
        {
            case D3D12_COMMAND_LIST_TYPE_DIRECT:
                sourceQueue = &GetQueue(QueueType::Graphics);
                break;
            case D3D12_COMMAND_LIST_TYPE_COMPUTE:
                sourceQueue = &GetQueue(QueueType::Compute);
                break;
            case D3D12_COMMAND_LIST_TYPE_COPY:
                sourceQueue = &GetQueue(QueueType::Copy);
                break;
            default:
                N_CORE_ASSERT(false, "Unknown submission type");
                break;
        }

        switch (aWaitType)
        {
            case ContextWaitType::Host:
                sourceQueue->WaitForFenceCPUBlocking(submission.fenceValue);
                break;
            case ContextWaitType::Graphics:
                GetQueue(QueueType::Graphics).InsertWaitForQueueFence(*sourceQueue, submission.fenceValue);
                break;
            case ContextWaitType::Compute:
                GetQueue(QueueType::Compute).InsertWaitForQueueFence(*sourceQueue, submission.fenceValue);
                break;
            case ContextWaitType::Copy:
                GetQueue(QueueType::Copy).InsertWaitForQueueFence(*sourceQueue, submission.fenceValue);
                break;
            default:
                N_CORE_ASSERT(false, "Unknown wait type");
                break;
        }
    }

    void Device::WaitForIdle()
    {
        for (auto& queue : myQueues)
        {
            if (queue) queue->WaitForIdle();
        }
    }

    std::unique_ptr<GraphicsContext> Device::CreateGraphicsContext()
    {
        return std::make_unique<GraphicsContext>(*this);
    }

    std::unique_ptr<RenderSurface> Device::CreateRenderSurface(const RenderSurfaceDesc& aDesc)
    {
        return std::make_unique<RenderSurface>(*this, aDesc);
    }

    std::unique_ptr<TextureResource> Device::CreateTexture(const TextureCreationDesc& aDesc)
    {
        N_CORE_ASSERT(aDesc.format != TextureFormat::Unknown, "Texture format must be specified");
        N_CORE_ASSERT(aDesc.width > 0 && aDesc.height > 0, "Texture dimensions must be non-zero");

        const bool hasSRV{ aDesc.HasFlag(TextureViewFlags::ShaderResource) };
        const bool hasRTV{ aDesc.HasFlag(TextureViewFlags::RenderTarget) };
        const bool hasDSV{ aDesc.HasFlag(TextureViewFlags::DepthStencil) };
        const bool hasUAV{ aDesc.HasFlag(TextureViewFlags::UnorderedAccess) };
        
        // Depth textures must be created as typeless so SRV and DSV can alias the same memory
        const DXGI_FORMAT resourceFormat{ hasDSV ? DepthFormatToTypeless(aDesc.format) : TextureFormatToDXGI(aDesc.format) };
        
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width              = aDesc.width;
        resourceDesc.Height             = aDesc.height;
        resourceDesc.DepthOrArraySize   = aDesc.depth > 1 ? aDesc.depth : aDesc.arraySize;
        resourceDesc.MipLevels          = aDesc.mipLevels;
        resourceDesc.Format             = resourceFormat;
        resourceDesc.SampleDesc.Count   = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Alignment          = 0;
        resourceDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;
        resourceDesc.Dimension          = aDesc.depth > 1 ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        
        if (hasRTV) resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if (hasDSV) resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        if (hasUAV) resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        
        // Determine initial state - DSV/RTV/UAV are immediately usable, others start as copy dest
        D3D12_RESOURCE_STATES initialState{ D3D12_RESOURCE_STATE_COMMON };
        if (hasRTV) initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
        if (hasDSV) initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        if (hasUAV) initialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        
        D3D12_CLEAR_VALUE clearValue{};
        D3D12_CLEAR_VALUE* clearValuePtr{ nullptr };
        
        if (hasRTV)
        {
            clearValue.Format  = TextureFormatToDXGI(aDesc.format);
            clearValue.Color[0] = 0.0f;
            clearValue.Color[1] = 0.0f;
            clearValue.Color[2] = 0.0f;
            clearValue.Color[3] = 1.0f;
            clearValuePtr = &clearValue;
        }
        else if (hasDSV)
        {
            clearValue.Format = TextureFormatToDXGI(aDesc.format);
            clearValue.DepthStencil.Depth = 0.0f;
            clearValue.DepthStencil.Stencil = 0;
            clearValuePtr = &clearValue;
        }
        
        D3D12MA::ALLOCATION_DESC allocationDesc{};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        
        auto texture{ std::make_unique<TextureResource>() };
        texture->state = initialState;
        texture->type = ResourceType::Texture;
        texture->isReady = hasRTV || hasDSV;
        
        NL_DX_VERIFY(myAllocator->CreateResource(
            &allocationDesc,
            &resourceDesc,
            initialState,
            clearValuePtr,
            &texture->allocation,
            IID_PPV_ARGS(&texture->resource)));
        
        // SRV - goes into bindless heap, heapIndex is what shaders use
        if (hasSRV)
        {
            texture->SRVDescriptor = myBindlessHeap->Allocate();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    
            if (hasDSV)
            {
                srvDesc.Format                        = DepthFormatToSRV(aDesc.format);
                srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels           = aDesc.mipLevels;
                srvDesc.Texture2D.MostDetailedMip     = 0;
                srvDesc.Texture2D.PlaneSlice          = 0;
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            }
            else if (aDesc.IsCubeMap())
            {
                srvDesc.Format                          = TextureFormatToDXGI(aDesc.format);
                srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
                srvDesc.TextureCube.MostDetailedMip     = 0;
                srvDesc.TextureCube.MipLevels           = aDesc.mipLevels;
                srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
            }
            else if (aDesc.depth > 1)
            {
                srvDesc.Format                        = TextureFormatToDXGI(aDesc.format);
                srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
                srvDesc.Texture3D.MostDetailedMip     = 0;
                srvDesc.Texture3D.MipLevels           = aDesc.mipLevels;
                srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
            }
            else if (aDesc.arraySize > 1)
            {
                srvDesc.Format                               = TextureFormatToDXGI(aDesc.format);
                srvDesc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MostDetailedMip       = 0;
                srvDesc.Texture2DArray.MipLevels             = aDesc.mipLevels;
                srvDesc.Texture2DArray.FirstArraySlice       = 0;
                srvDesc.Texture2DArray.ArraySize             = aDesc.arraySize;
                srvDesc.Texture2DArray.PlaneSlice            = 0;
                srvDesc.Texture2DArray.ResourceMinLODClamp   = 0.0f;
            }
            else
            {
                srvDesc.Format                        = TextureFormatToDXGI(aDesc.format);
                srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels           = aDesc.mipLevels;
                srvDesc.Texture2D.MostDetailedMip     = 0;
                srvDesc.Texture2D.PlaneSlice          = 0;
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            }
    
            myDevice->CreateShaderResourceView(texture->resource, &srvDesc, texture->SRVDescriptor.CPUHandle);
        }
        
        // RTV - goes into RTV staging heap
        if (hasRTV)
        {
            texture->RTVDescriptor = myRTVHeap->Allocate();
            myDevice->CreateRenderTargetView(texture->resource, nullptr, texture->RTVDescriptor.CPUHandle);
        }
        
        // DSV - goes into DSV staging heap
        if (hasDSV)
        {
            texture->DSVDescriptor = myDSVHeap->Allocate();

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
            dsvDesc.Format             = TextureFormatToDXGI(aDesc.format);
            dsvDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Flags              = D3D12_DSV_FLAG_NONE;
            dsvDesc.Texture2D.MipSlice = 0;

            myDevice->CreateDepthStencilView(texture->resource, &dsvDesc, texture->DSVDescriptor.CPUHandle);
        }
        
        // UAV - goes into bindless heap
        if (hasUAV)
        {
            texture->UAVDescriptor = myBindlessHeap->Allocate();
            myDevice->CreateUnorderedAccessView(texture->resource, nullptr, nullptr, texture->UAVDescriptor.CPUHandle);
        }
        
        if (!aDesc.debugName.empty())
        {
            const std::wstring wideName(aDesc.debugName.begin(), aDesc.debugName.end());
            N_D3D12_SET_NAME_W(texture->resource, wideName.c_str());
        }

        NL_TRACE(GCoreLogger, "texture created '{}' [{}x{}]", aDesc.debugName, aDesc.width, aDesc.height);
        
        return texture;
    }

    std::unique_ptr<BufferResource> Device::CreateBuffer(const BufferCreationDesc& aDesc)
    {
        N_CORE_ASSERT(aDesc.size > 0, "Buffer size must be non-zero");
        N_CORE_ASSERT(aDesc.access != BufferAccessFlags::GpuOnly || !aDesc.HasFlag(BufferViewFlags::ConstantBuffer),
            "Constant buffers must be CpuToGpu access");
        
        const bool hasCBV{ aDesc.HasFlag(BufferViewFlags::ConstantBuffer) };
        const bool hasSRV{ aDesc.HasFlag(BufferViewFlags::ShaderResource) };
        const bool hasUAV{ aDesc.HasFlag(BufferViewFlags::UnorderedAccess) };
        const bool isUpload{ aDesc.access == BufferAccessFlags::CpuToGpu };
        const bool isReadback{ aDesc.access == BufferAccessFlags::GpuToCpu };
        
        // CBVs must be 256-byte aligned, apply to all buffers for safety
        const uint64_t alignedSize{ (aDesc.size + 255) & ~255ull };
        
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width              = alignedSize;
        resourceDesc.Height             = 1;
        resourceDesc.DepthOrArraySize   = 1;
        resourceDesc.MipLevels          = 1;
        resourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count   = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags              = hasUAV
            ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
            : D3D12_RESOURCE_FLAG_NONE;
        
        D3D12_RESOURCE_STATES initialState{ D3D12_RESOURCE_STATE_COPY_DEST };
        if (isUpload)   initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        if (isReadback) initialState = D3D12_RESOURCE_STATE_COPY_DEST;
        
        D3D12MA::ALLOCATION_DESC allocDesc{};
        switch (aDesc.access)
        {
            case BufferAccessFlags::GpuOnly:  allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;  break;
            case BufferAccessFlags::CpuToGpu: allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;   break;
            case BufferAccessFlags::GpuToCpu: allocDesc.HeapType = D3D12_HEAP_TYPE_READBACK; break;
            default:
                N_CORE_ASSERT(false, "Unknown buffer access flag");
                break;
        }
        
        auto buffer{ std::make_unique<BufferResource>() };
        buffer->state = initialState;
        buffer->stride = aDesc.stride;
        
        NL_DX_VERIFY(myAllocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            initialState,
            nullptr,
            &buffer->allocation,
            IID_PPV_ARGS(&buffer->resource)));

        buffer->gpuAddress = buffer->resource->GetGPUVirtualAddress();
        
        if (isUpload || isReadback)
        {
            NL_DX_VERIFY(buffer->resource->Map(0, nullptr, reinterpret_cast<void**>(&buffer->mappedData)));
        }
        
        const uint32_t numElements{ aDesc.stride > 0 ? static_cast<uint32_t>(aDesc.size / aDesc.stride) : static_cast<uint32_t>(aDesc.size / 4) };
        
        // CBV - bindless heap
        if (hasCBV)
        {
            buffer->CBVDescriptor = myBindlessHeap->Allocate();

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
            cbvDesc.BufferLocation = buffer->gpuAddress;
            cbvDesc.SizeInBytes    = static_cast<uint32_t>(alignedSize);

            myDevice->CreateConstantBufferView(&cbvDesc, buffer->CBVDescriptor.CPUHandle);
        }
        
        // SRV - bindless heap
        if (hasSRV)
        {
            buffer->SRVDescriptor = myBindlessHeap->Allocate();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.ViewDimension               = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Shader4ComponentMapping     = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Buffer.FirstElement         = 0;
            srvDesc.Buffer.NumElements          = numElements;

            if (aDesc.isRawAccess)
            {
                srvDesc.Format                  = DXGI_FORMAT_R32_TYPELESS;
                srvDesc.Buffer.StructureByteStride = 0;
                srvDesc.Buffer.Flags            = D3D12_BUFFER_SRV_FLAG_RAW;
            }
            else
            {
                srvDesc.Format                  = DXGI_FORMAT_UNKNOWN;
                srvDesc.Buffer.StructureByteStride = aDesc.stride;
                srvDesc.Buffer.Flags            = D3D12_BUFFER_SRV_FLAG_NONE;
            }

            myDevice->CreateShaderResourceView(buffer->resource, &srvDesc, buffer->SRVDescriptor.CPUHandle);
        }
        
        // UAV — bindless heap
        if (hasUAV)
        {
            buffer->UAVDescriptor = myBindlessHeap->Allocate();

            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
            uavDesc.ViewDimension               = D3D12_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement         = 0;
            uavDesc.Buffer.NumElements          = numElements;
            uavDesc.Buffer.CounterOffsetInBytes = 0;

            if (aDesc.isRawAccess)
            {
                uavDesc.Format                     = DXGI_FORMAT_R32_TYPELESS;
                uavDesc.Buffer.StructureByteStride = 0;
                uavDesc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_RAW;
            }
            else
            {
                uavDesc.Format                     = DXGI_FORMAT_UNKNOWN;
                uavDesc.Buffer.StructureByteStride = aDesc.stride;
                uavDesc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
            }

            myDevice->CreateUnorderedAccessView(buffer->resource, nullptr, &uavDesc, buffer->UAVDescriptor.CPUHandle);
        }
        
        if (!aDesc.debugName.empty())
        {
            const std::wstring wideName(aDesc.debugName.begin(), aDesc.debugName.end());
            N_D3D12_SET_NAME_W(buffer->resource, wideName.c_str());
        }

        NL_TRACE(GCoreLogger, "buffer created '{}' [{} bytes]", aDesc.debugName, alignedSize);

        return buffer;
    }

    void Device::DestroyRenderSurface(std::unique_ptr<RenderSurface> aSurface)
    {
        N_CORE_ASSERT(aSurface != nullptr, "Destroying null render surface");
        NL_TRACE(GCoreLogger, "waiting for GPU idle before releasing render surface");
        WaitForIdle();
        aSurface.reset();
        NL_TRACE(GCoreLogger, "render surface destroyed");
    }

    void Device::DestroyTexture(std::unique_ptr<TextureResource> aTexture)
    {
        N_CORE_ASSERT(aTexture != nullptr, "Destroying null texture");
        myDestructionQueues[myFrameIndex].texturesToDestroy.push_back(std::move(aTexture));
    }

    void Device::DestroyBuffer(std::unique_ptr<BufferResource> aBuffer)
    {
        N_CORE_ASSERT(aBuffer != nullptr, "Destroying null buffer");
        myDestructionQueues[myFrameIndex].buffersToDestroy.push_back(std::move(aBuffer));
    }

    void Device::DestroyContext(std::unique_ptr<Context> aContext)
    {
        N_CORE_ASSERT(aContext != nullptr, "Destroying null context");
        myDestructionQueues[myFrameIndex].contextsToDestroy.push_back(std::move(aContext));
    }

    void Device::InitDebugLayer()
    {
#ifndef N_SHIPPING
        
        ID3D12DeviceRemovedExtendedDataSettings1* dredSettings{ nullptr };
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings))))
        {
            dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dredSettings->SetWatsonDumpEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            SafeRelease(dredSettings);
            NL_TRACE(GCoreLogger, "DRED enabled");
        }
        else
        {
            NL_WARN(GCoreLogger, "Failed to enable DRED");
        }
        
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&myDebugController))))
        {
            myDebugController->EnableDebugLayer();
#ifdef N_DEBUG
            myDebugController->SetEnableGPUBasedValidation(TRUE);
            NL_INFO(GCoreLogger, "GPU based validation is enabled. This will considerably slow down the renderer!");
#endif
            NL_TRACE(GCoreLogger, "debug layer enabled");
        }
        else
        {
            NL_WARN(GCoreLogger, "failed to enable debug layer");
        }
#endif
    }

    void Device::InitInfoQueue()
    {
#ifdef N_DEBUG
        ID3D12InfoQueue* infoQueue;
        if (SUCCEEDED(myDevice->QueryInterface(IID_PPV_ARGS(&infoQueue))))
        {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
            
            // Filter out the expected live device warning on shutdown
            D3D12_MESSAGE_ID filteredMessages[]
            {
                D3D12_MESSAGE_ID_LIVE_DEVICE
            };

            D3D12_INFO_QUEUE_FILTER filter{};
            filter.DenyList.NumIDs  = static_cast<UINT>(std::size(filteredMessages));
            filter.DenyList.pIDList = filteredMessages;
            infoQueue->AddStorageFilterEntries(&filter);
        }
        
        // Message callback via ID3D12InfoQueue1 - routes to logger
        ID3D12InfoQueue1* infoQueue1;
        if (SUCCEEDED(myDevice->QueryInterface(IID_PPV_ARGS(&infoQueue1))))
        {
            infoQueue1->RegisterMessageCallback(
                [](D3D12_MESSAGE_CATEGORY, const D3D12_MESSAGE_SEVERITY aSeverity,
                   D3D12_MESSAGE_ID, LPCSTR aDescription, void*)
                {
                    switch (aSeverity)
                    {
                        case D3D12_MESSAGE_SEVERITY_CORRUPTION:
                        case D3D12_MESSAGE_SEVERITY_ERROR:
                        {
                            NL_ERROR(GCoreLogger, "[D3D12] {}", aDescription);
                            break;
                        }
                        case D3D12_MESSAGE_SEVERITY_WARNING:
                        {
                            NL_WARN(GCoreLogger, "[D3D12] {}", aDescription);
                            break;
                        }
                        case D3D12_MESSAGE_SEVERITY_INFO:
                        {
                            NL_INFO(GCoreLogger, "[D3D12] {}", aDescription);
                            break;
                        }
                        default:
                        {
                            NL_TRACE(GCoreLogger, "[D3D12] {}", aDescription);
                            break;
                        }
                    }
                },
                D3D12_MESSAGE_CALLBACK_FLAG_NONE,
                nullptr,
                &myCallbackCookie);
        }
        
        SafeRelease(infoQueue);
        SafeRelease(infoQueue1);

        NL_TRACE(GCoreLogger, "info queue configured");
#endif
    }

    void Device::SelectAdapter()
    {
        for (UINT i{ 0 }; myFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&myAdapter)) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            // Pick first adapter that supports our minimum feature level
            if (SUCCEEDED(D3D12CreateDevice(myAdapter, MIN_FEATURE_LEVEL, __uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
            SafeRelease(myAdapter);
        }

        if (myAdapter == nullptr)
        {
            NL_FATAL(GCoreLogger, "no adapter found that supports feature level 12_0");
        }

        DXGI_ADAPTER_DESC1 desc{};
        myAdapter->GetDesc1(&desc);

        auto toMB = [](const SIZE_T bytes)
        {
            return static_cast<size_t>(bytes / (1024 * 1024));
        };
        
        char adapterName[128]{};
        WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, adapterName, sizeof(adapterName), nullptr, nullptr);
        NL_INFO(GCoreLogger, "selected adapter '{}' VRAM: {} MB", adapterName, toMB(desc.DedicatedVideoMemory));
    }

    void Device::InitAllocator()
    {
        D3D12MA::ALLOCATOR_DESC desc{};
        desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
        desc.pDevice = myDevice;
        desc.pAdapter = myAdapter;

        if (FAILED(D3D12MA::CreateAllocator(&desc, &myAllocator)))
        {
            NL_FATAL(GCoreLogger, "failed to create D3D12MA allocator");
        }

        NL_TRACE(GCoreLogger, "allocator initialized");
    }

    void Device::InitDescriptorHeaps()
    {
        myBindlessHeap = std::make_unique<FreeListDescriptorHeap>(myDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, HeapCapacity::BINDLESS, true);
        N_D3D12_SET_NAME(myBindlessHeap->GetHeap(), "Bindless CBV/SRV/UAV Heap");

        mySamplerHeap = std::make_unique<FreeListDescriptorHeap>(myDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, HeapCapacity::SAMPLER, true);
        N_D3D12_SET_NAME(mySamplerHeap->GetHeap(), "Sampler Heap");

        myRTVHeap = std::make_unique<FreeListDescriptorHeap>(myDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, HeapCapacity::RTV, false);
        N_D3D12_SET_NAME(myRTVHeap->GetHeap(), "RTV Staging Heap");

        myDSVHeap = std::make_unique<FreeListDescriptorHeap>(myDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, HeapCapacity::DSV, false);
        N_D3D12_SET_NAME(myDSVHeap->GetHeap(), "DSV Staging Heap");

        NL_TRACE(GCoreLogger, "descriptor heaps initialized");
    }

    void Device::CheckFeatureSupport() const
    {
        // Shader Model - need 6.6 for ResourceDescriptorHeap indexing
        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{};
        shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
        if (FAILED(myDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))) || shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_6)
        {
            NL_FATAL(GCoreLogger, "Shader Model 6.6 is required for bindless rendering");
        }
        NL_INFO(GCoreLogger, "Shader Model 6.6 supported");
        
        // Resource binding tier - need Tier 3 for full bindless
        D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
        if (FAILED(myDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options))))
        {
            NL_FATAL(GCoreLogger, "Failed to query D3D12 options");
        }
        if (options.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3)
        {
            NL_FATAL(GCoreLogger, "Resource Binding Tier 3 is required for bindless rendering");
        }
        NL_INFO(GCoreLogger, "Resource Binding Tier 3 supported");
        
        // Ray tracing - Tier 1.1 for inline ray tracing in compute/pixel shaders
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
        if (FAILED(myDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))))
        {
            NL_FATAL(GCoreLogger, "Failed to query D3D12 options5");
        }
        if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_1)
        {
            NL_WARN(GCoreLogger, "Ray tracing Tier 1.1 not supported on this device");
        }
        else
        {
            NL_INFO(GCoreLogger, "Ray tracing Tier 1.1 supported");
        }
        
        // Mesh shaders
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7{};
        if (FAILED(myDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7))))
        {
            NL_FATAL(GCoreLogger, "Failed to query D3D12 options7");
        }
        if (options7.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
        {
            NL_WARN(GCoreLogger, "Mesh shaders not supported on this device");
        }
        else
        {
            NL_INFO(GCoreLogger, "Mesh shaders supported");
        }
    }

    void Device::ProcessDestructions(const uint32_t aFrameIndex)
    {
        [[maybe_unused]] auto& queue{ myDestructionQueues[aFrameIndex] };
        
        for (auto& texture : queue.texturesToDestroy)
        {
            if (texture->SRVDescriptor.IsValid()) myBindlessHeap->Free(texture->SRVDescriptor);
            if (texture->RTVDescriptor.IsValid()) myRTVHeap->Free(texture->RTVDescriptor);
            if (texture->DSVDescriptor.IsValid()) myDSVHeap->Free(texture->DSVDescriptor);
            if (texture->UAVDescriptor.IsValid()) myBindlessHeap->Free(texture->UAVDescriptor);
            
            if (texture->allocation != nullptr)
            {
                SafeRelease(texture->allocation);
            }

            SafeRelease(texture->resource);
        }
        for (auto& buffer : queue.buffersToDestroy)
        {
            if (buffer->mappedData != nullptr)
            {
                buffer->resource->Unmap(0, nullptr);
            }

            if (buffer->CBVDescriptor.IsValid()) myBindlessHeap->Free(buffer->CBVDescriptor);
            if (buffer->SRVDescriptor.IsValid()) myBindlessHeap->Free(buffer->SRVDescriptor);
            if (buffer->UAVDescriptor.IsValid()) myBindlessHeap->Free(buffer->UAVDescriptor);

            if (buffer->allocation != nullptr)
            {
                SafeRelease(buffer->allocation);
            }

            SafeRelease(buffer->resource);
        }
        
        queue.texturesToDestroy.clear();
        queue.buffersToDestroy.clear();
        queue.contextsToDestroy.clear(); // unique_ptr destructor handles cleanup
    }
}
