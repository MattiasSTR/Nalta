#include "npch.h"
#include "Nalta/RHI/D3D12/D3D12Device.h"

#include "Nalta/RHI/D3D12/D3D12RenderSurface.h"
#include "Nalta/RHI/D3D12/Common/D3D12Queue.h"
#include "Nalta/RHI/D3D12/Contexts/D3D12ComputeContext.h"
#include "Nalta/RHI/D3D12/Contexts/D3D12GraphicsContext.h"
#include "Nalta/RHI/D3D12/Contexts/D3D12UploadContext.h"
#include "Nalta/RHI/D3D12/Resources/D3D12PipelineStateObject.h"

namespace Nalta::RHI::D3D12
{
    namespace
    {
        std::wstring ToWide(const std::string& aStr)
        {
            return std::wstring(aStr.begin(), aStr.end());
        }
        
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
        
        const wchar_t* ShaderStageToProfile(const ShaderStage aStage)
        {
            switch (aStage)
            {
                case ShaderStage::Vertex:        return L"vs_6_6";
                case ShaderStage::Pixel:         return L"ps_6_6";
                case ShaderStage::Compute:       return L"cs_6_6";
                case ShaderStage::Mesh:          return L"ms_6_6";
                case ShaderStage::Amplification: return L"as_6_6";
                default:
                    N_CORE_ASSERT(false, "Unknown shader stage");
                    return L"";
            }
        }
        
        D3D12_CULL_MODE ToDXGICullMode(const CullMode aCullMode)
        {
            switch (aCullMode)
            {
                case CullMode::None:  return D3D12_CULL_MODE_NONE;
                case CullMode::Front: return D3D12_CULL_MODE_FRONT;
                case CullMode::Back:  return D3D12_CULL_MODE_BACK;
                default:
                    N_CORE_ASSERT(false, "Unknown CullMode");
                    return D3D12_CULL_MODE_NONE;
            }
        }
        
        D3D12_FILL_MODE ToDXGIFillMode(const FillMode aFillMode)
        {
            switch (aFillMode)
            {
                case FillMode::Solid:     return D3D12_FILL_MODE_SOLID;
                case FillMode::Wireframe: return D3D12_FILL_MODE_WIREFRAME;
                default:
                    N_CORE_ASSERT(false, "Unknown FillMode");
                    return D3D12_FILL_MODE_SOLID;
            }
        }
        
        D3D12_COMPARISON_FUNC ToDXGIComparisonFunc(const DepthCompare aCompare)
        {
            switch (aCompare)
            {
                case DepthCompare::Never:        return D3D12_COMPARISON_FUNC_NEVER;
                case DepthCompare::Less:         return D3D12_COMPARISON_FUNC_LESS;
                case DepthCompare::LessEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
                case DepthCompare::Equal:        return D3D12_COMPARISON_FUNC_EQUAL;
                case DepthCompare::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
                case DepthCompare::Greater:      return D3D12_COMPARISON_FUNC_GREATER;
                case DepthCompare::NotEqual:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
                case DepthCompare::Always:       return D3D12_COMPARISON_FUNC_ALWAYS;
                default:
                    N_CORE_ASSERT(false, "Unknown DepthCompare");
                    return D3D12_COMPARISON_FUNC_LESS;
            }
        }
        
        D3D12_BLEND ToDXGIBlendFactor(const BlendFactor aFactor)
        {
            switch (aFactor)
            {
                case BlendFactor::Zero:             return D3D12_BLEND_ZERO;
                case BlendFactor::One:              return D3D12_BLEND_ONE;
                case BlendFactor::SrcAlpha:         return D3D12_BLEND_SRC_ALPHA;
                case BlendFactor::OneMinusSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
                case BlendFactor::SrcColor:         return D3D12_BLEND_SRC_COLOR;
                case BlendFactor::OneMinusSrcColor: return D3D12_BLEND_INV_SRC_COLOR;
                default:
                    N_CORE_ASSERT(false, "Unknown BlendFactor");
                    return D3D12_BLEND_ZERO;
            }
        }
        
        D3D12_BLEND_OP ToDXGIBlendOp(const BlendOp aOp)
        {
            switch (aOp)
            {
                case BlendOp::Add:             return D3D12_BLEND_OP_ADD;
                case BlendOp::Subtract:        return D3D12_BLEND_OP_SUBTRACT;
                case BlendOp::ReverseSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
                case BlendOp::Min:             return D3D12_BLEND_OP_MIN;
                case BlendOp::Max:             return D3D12_BLEND_OP_MAX;
                default:
                    N_CORE_ASSERT(false, "Unknown BlendOp");
                    return D3D12_BLEND_OP_ADD;
            }
        }
        
        D3D12_PRIMITIVE_TOPOLOGY_TYPE ToDXGITopologyType(const PrimitiveTopology aTopology)
        {
            switch (aTopology)
            {
                case PrimitiveTopology::TriangleList:
                case PrimitiveTopology::TriangleStrip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                case PrimitiveTopology::LineList:
                case PrimitiveTopology::LineStrip:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
                case PrimitiveTopology::PointList:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
                default:
                    N_CORE_ASSERT(false, "Unknown PrimitiveTopology");
                    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            }
        }
        
        D3D12_RASTERIZER_DESC BuildRasterizerDesc(const RasterizerDesc& aDesc)
        {
            D3D12_RASTERIZER_DESC desc{};
            desc.FillMode              = ToDXGIFillMode(aDesc.fillMode);
            desc.CullMode              = ToDXGICullMode(aDesc.cullMode);
            desc.FrontCounterClockwise = aDesc.frontCCW ? TRUE : FALSE;
            desc.DepthBias             = static_cast<INT>(aDesc.depthBias);
            desc.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
            desc.SlopeScaledDepthBias  = aDesc.slopeScaledDepthBias;
            desc.DepthClipEnable       = TRUE;
            desc.MultisampleEnable     = FALSE;
            desc.AntialiasedLineEnable = FALSE;
            desc.ForcedSampleCount     = 0;
            desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
            return desc;
        }
        
        D3D12_BLEND_DESC BuildBlendDesc(const BlendDesc& aDesc)
        {
            D3D12_BLEND_DESC desc{};
            desc.AlphaToCoverageEnable  = FALSE;
            desc.IndependentBlendEnable = TRUE;

            for (uint32_t i{ 0 }; i < 8; ++i)
            {
                const auto& rt{ aDesc.renderTargets[i] };
                auto& d3dRT{ desc.RenderTarget[i] };

                d3dRT.BlendEnable           = rt.enabled ? TRUE : FALSE;
                d3dRT.LogicOpEnable         = FALSE;
                d3dRT.SrcBlend              = ToDXGIBlendFactor(rt.srcBlend);
                d3dRT.DestBlend             = ToDXGIBlendFactor(rt.dstBlend);
                d3dRT.BlendOp               = ToDXGIBlendOp(rt.blendOp);
                d3dRT.SrcBlendAlpha         = ToDXGIBlendFactor(rt.srcBlendAlpha);
                d3dRT.DestBlendAlpha        = ToDXGIBlendFactor(rt.dstBlendAlpha);
                d3dRT.BlendOpAlpha          = ToDXGIBlendOp(rt.blendOpAlpha);
                d3dRT.LogicOp               = D3D12_LOGIC_OP_NOOP;
                d3dRT.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            }

            return desc;
        }
        
        D3D12_DEPTH_STENCIL_DESC BuildDepthStencilDesc(const DepthDesc& aDesc)
        {
            D3D12_DEPTH_STENCIL_DESC desc{};
            desc.DepthEnable    = aDesc.depthEnabled ? TRUE : FALSE;
            desc.DepthWriteMask = aDesc.depthWrite
                ? D3D12_DEPTH_WRITE_MASK_ALL
                : D3D12_DEPTH_WRITE_MASK_ZERO;
            desc.DepthFunc      = ToDXGIComparisonFunc(aDesc.compareFunc);
            desc.StencilEnable  = aDesc.stencilEnabled ? TRUE : FALSE;

            // Default stencil ops - expand when stencil is actually used
            constexpr D3D12_DEPTH_STENCILOP_DESC defaultStencilOp
            {
                .StencilFailOp      = D3D12_STENCIL_OP_KEEP,
                .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                .StencilPassOp      = D3D12_STENCIL_OP_KEEP,
                .StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS,
            };

            desc.StencilReadMask  = D3D12_DEFAULT_STENCIL_READ_MASK;
            desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
            desc.FrontFace        = defaultStencilOp;
            desc.BackFace         = defaultStencilOp;

            return desc;
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
        InitRootSignature();
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
        
        if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&myDxcCompiler))))
        {
            NL_FATAL(GCoreLogger, "failed to create DXC compiler");
        }

        if (FAILED(myDxcUtils->CreateDefaultIncludeHandler(&myDxcIncludeHandler)))
        {
            NL_FATAL(GCoreLogger, "failed to create DXC include handler");
        }

        NL_TRACE(GCoreLogger, "shader compiler initialized");

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
        
        SafeRelease(myRootSignature);
        
        myBindlessHeap.reset();
        mySamplerHeap.reset();
        myRTVHeap.reset();
        myDSVHeap.reset();
        
        SafeRelease(myAllocator);
        SafeRelease(myDxcIncludeHandler);
        SafeRelease(myDxcCompiler);
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

    std::unique_ptr<ComputeContext> Device::CreateComputeContext()
    {
        return std::make_unique<ComputeContext>(*this);
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
        
        // UAV - bindless heap
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
        
        buffer->size = aDesc.size;
        buffer->isReady = (isUpload || isReadback); // CpuToGpu/GpuToCpu are immediately usable
        
        if (!aDesc.debugName.empty())
        {
            const std::wstring wideName(aDesc.debugName.begin(), aDesc.debugName.end());
            N_D3D12_SET_NAME_W(buffer->resource, wideName.c_str());
        }

        NL_TRACE(GCoreLogger, "buffer created '{}' [{} bytes]", aDesc.debugName, alignedSize);

        return buffer;
    }

    std::unique_ptr<Shader> Device::CreateShader(const ShaderDesc& aDesc)
    {
        N_CORE_ASSERT(!aDesc.stages.empty(), "ShaderDesc has no stages");

        if (!std::filesystem::exists(aDesc.filePath))
        {
            NL_ERROR(GCoreLogger, "shader file not found: '{}'", aDesc.filePath.string());
            return nullptr;
        }

        auto shader{ std::make_unique<Shader>() };
        shader->debugName = aDesc.debugName;
        
        for (const auto& stageDesc : aDesc.stages)
        {
            IDxcBlobEncoding* sourceBlob{ nullptr };
            if (FAILED(myDxcUtils->LoadFile(
                aDesc.filePath.wstring().c_str(), nullptr, &sourceBlob)))
            {
                NL_ERROR(GCoreLogger, "failed to load shader '{}'", aDesc.filePath.string());
                return nullptr;
            }
            
            const DxcBuffer sourceBuffer
            {
                .Ptr      = sourceBlob->GetBufferPointer(),
                .Size     = sourceBlob->GetBufferSize(),
                .Encoding = DXC_CP_ACP
            };
            
            std::vector<LPCWSTR> args;
            
            // Row-major matrices
            args.push_back(L"-Zpr");
            
            // Filename for error messages
            const std::wstring fileName{ aDesc.filePath.wstring() };
            args.push_back(fileName.c_str());

            // Entry point
            args.push_back(L"-E");
            const std::wstring entryPoint{ ToWide(stageDesc.entryPoint) };
            args.push_back(entryPoint.c_str());

            // Target profile
            args.push_back(L"-T");
            args.push_back(ShaderStageToProfile(stageDesc.stage));

            // Include directory - parent of source file
            args.push_back(L"-I");
            const std::wstring includeDir{ aDesc.filePath.parent_path().wstring() };
            args.push_back(includeDir.c_str());
            
#ifndef N_SHIPPING
            args.push_back(L"-Zi");
            args.push_back(L"-Qembed_debug");
            args.push_back(L"-Od");
#else
            args.push_back(L"-O3");
#endif

            std::vector<std::wstring> defineStrings;
            for (const auto& [name, value] : aDesc.defines)
            {
                defineStrings.push_back(ToWide(name + "=" + value));
                args.push_back(L"-D");
                args.push_back(defineStrings.back().c_str());
            }
            
            IDxcResult* result{ nullptr };
            if (FAILED(myDxcCompiler->Compile(
                &sourceBuffer,
                args.data(),
                static_cast<UINT32>(args.size()),
                myDxcIncludeHandler,
                IID_PPV_ARGS(&result))))
            {
                SafeRelease(sourceBlob);
                NL_ERROR(GCoreLogger, "DXC compile call failed for '{}'", aDesc.filePath.string());
                return nullptr;
            }
            
            IDxcBlobUtf8* errors{ nullptr };
            result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
            if (errors && errors->GetStringLength() > 0)
            {
                NL_ERROR(GCoreLogger, "shader '{}' ({}):\n{}",
                    aDesc.filePath.string(),
                    stageDesc.entryPoint,
                    errors->GetStringPointer());
            }
            SafeRelease(errors);
            
            HRESULT status{};
            result->GetStatus(&status);
            if (FAILED(status))
            {
                SafeRelease(result);
                SafeRelease(sourceBlob);
                NL_ERROR(GCoreLogger, "shader compilation failed '{}' ({})", aDesc.filePath.string(), stageDesc.entryPoint);
                return nullptr;
            }

            IDxcBlob* bytecodeBlob{ nullptr };
            result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&bytecodeBlob), nullptr);
            if (!bytecodeBlob || bytecodeBlob->GetBufferSize() == 0)
            {
                NL_ERROR(GCoreLogger, "empty bytecode for '{}' ({})", aDesc.filePath.string(), stageDesc.entryPoint);
                SafeRelease(bytecodeBlob);
                SafeRelease(result);
                SafeRelease(sourceBlob);
                return nullptr;
            }
            
            CompiledStage compiled{};
            compiled.stage = stageDesc.stage;
            const auto* data{ static_cast<const uint8_t*>(bytecodeBlob->GetBufferPointer()) };
            compiled.bytecode.assign(data, data + bytecodeBlob->GetBufferSize());
            shader->stages.push_back(std::move(compiled));
            
            SafeRelease(bytecodeBlob);
            SafeRelease(result);
            SafeRelease(sourceBlob);
            
            NL_TRACE(GCoreLogger, "compiled '{}' ({})", aDesc.filePath.string(), stageDesc.entryPoint);
        }
        
        NL_INFO(GCoreLogger, "shader created '{}'", aDesc.debugName);
        return shader;
    }

    std::unique_ptr<PipelineStateObject> Device::CreateGraphicsPipeline(const GraphicsPipelineDesc& aDesc)
    {
        N_CORE_ASSERT(aDesc.shader != nullptr, "GraphicsPipelineDesc has no shader");
        N_CORE_ASSERT(aDesc.shader->HasStage(RHI::ShaderStage::Vertex), "Graphics pipeline requires a vertex shader");
        N_CORE_ASSERT(!aDesc.renderTargetFormats.empty() || aDesc.depthFormat != RHI::TextureFormat::Unknown,
            "Pipeline must have at least one render target or a depth target");
        
        const CompiledStage* vs{ aDesc.shader->GetStage(ShaderStage::Vertex) };
        const CompiledStage* ps{ aDesc.shader->GetStage(ShaderStage::Pixel)  };
        
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature        = myRootSignature;
        psoDesc.NodeMask              = 0;
        psoDesc.SampleMask            = UINT_MAX;
        psoDesc.PrimitiveTopologyType = ToDXGITopologyType(aDesc.topology);
        psoDesc.RasterizerState       = BuildRasterizerDesc(aDesc.rasterizer);
        psoDesc.BlendState            = BuildBlendDesc(aDesc.blend);
        psoDesc.DepthStencilState     = BuildDepthStencilDesc(aDesc.depth);
        psoDesc.SampleDesc            = { 1, 0 };

        // No input layout - fully bindless, vertex data comes from buffers
        psoDesc.InputLayout = { nullptr, 0 };
        
        if (vs != nullptr)
        {
            psoDesc.VS.pShaderBytecode = vs->GetData();
            psoDesc.VS.BytecodeLength  = vs->GetSize();
        }
        
        if (ps != nullptr)
        {
            psoDesc.PS.pShaderBytecode = ps->GetData();
            psoDesc.PS.BytecodeLength  = ps->GetSize();
        }

        // Render target formats
        psoDesc.NumRenderTargets = static_cast<UINT>(aDesc.renderTargetFormats.size());
        for (uint32_t i{ 0 }; i < aDesc.renderTargetFormats.size(); ++i)
        {
            psoDesc.RTVFormats[i] = TextureFormatToDXGI(aDesc.renderTargetFormats[i]);
        }

        // Depth format
        psoDesc.DSVFormat = aDesc.depthFormat != TextureFormat::Unknown ? TextureFormatToDXGI(aDesc.depthFormat) : DXGI_FORMAT_UNKNOWN;

        auto pso{ std::make_unique<PipelineStateObject>() };
        pso->isCompute  = false;
        pso->debugName  = aDesc.debugName;

        if (FAILED(myDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso->pipelineState))))
        {
            NL_ERROR(GCoreLogger, "failed to create graphics pipeline '{}'", aDesc.debugName);
            return nullptr;
        }
        
        if (!aDesc.debugName.empty())
        {
            const std::wstring wideName(aDesc.debugName.begin(), aDesc.debugName.end());
            N_D3D12_SET_NAME_W(pso->pipelineState, wideName.c_str());
        }
        
        NL_TRACE(GCoreLogger, "graphics pipeline created '{}'", aDesc.debugName);
        return pso;
    }

    std::unique_ptr<PipelineStateObject> Device::CreateComputePipeline(const ComputePipelineDesc& aDesc)
    {
        N_CORE_ASSERT(aDesc.shader != nullptr, "ComputePipelineDesc has no shader");
        N_CORE_ASSERT(aDesc.shader->HasStage(RHI::ShaderStage::Compute), "Compute pipeline requires a compute shader");
        
        const CompiledStage* cs{ aDesc.shader->GetStage(ShaderStage::Compute) };

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature      = myRootSignature;
        psoDesc.NodeMask            = 0;
        psoDesc.CS.pShaderBytecode  = cs->GetData();
        psoDesc.CS.BytecodeLength   = cs->GetSize();

        auto pso{ std::make_unique<PipelineStateObject>() };
        pso->isCompute = true;
        pso->debugName = aDesc.debugName;
        
        if (FAILED(myDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso->pipelineState))))
        {
            NL_ERROR(GCoreLogger, "failed to create compute pipeline '{}'", aDesc.debugName);
            return nullptr;
        }

        if (!aDesc.debugName.empty())
        {
            const std::wstring wideName(aDesc.debugName.begin(), aDesc.debugName.end());
            N_D3D12_SET_NAME_W(pso->pipelineState, wideName.c_str());
        }

        NL_TRACE(GCoreLogger, "compute pipeline created '{}'", aDesc.debugName);
        return pso;
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

    void Device::DestroyPipeline(std::unique_ptr<PipelineStateObject> aPipeline)
    {
        N_CORE_ASSERT(aPipeline != nullptr, "Destroying null pipeline");
        myDestructionQueues[myFrameIndex].pipelinesToDestroy.push_back(std::move(aPipeline));
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

    void Device::InitRootSignature()
    {
        // Root parameter 0 - push constants, per-draw bindless indices
        // Root parameter 1 - per-pass/per-frame CBV, set once per pass
        D3D12_ROOT_PARAMETER1 rootParameters[2]{};

        // Push constants — 16 x uint32, visible to all shader stages
        rootParameters[static_cast<uint32_t>(RootParameter::Constants)].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[static_cast<uint32_t>(RootParameter::Constants)].ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[static_cast<uint32_t>(RootParameter::Constants)].Constants.ShaderRegister = 0;
        rootParameters[static_cast<uint32_t>(RootParameter::Constants)].Constants.RegisterSpace  = 0;
        rootParameters[static_cast<uint32_t>(RootParameter::Constants)].Constants.Num32BitValues = ROOT_CONSTANT_COUNT;

        // Per-pass CBV - root descriptor, GPU virtual address set directly
        rootParameters[static_cast<uint32_t>(RootParameter::PassCBV)].ParameterType              = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[static_cast<uint32_t>(RootParameter::PassCBV)].ShaderVisibility           = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[static_cast<uint32_t>(RootParameter::PassCBV)].Descriptor.ShaderRegister  = 1;
        rootParameters[static_cast<uint32_t>(RootParameter::PassCBV)].Descriptor.RegisterSpace   = 0;
        rootParameters[static_cast<uint32_t>(RootParameter::PassCBV)].Descriptor.Flags           = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.Version                    = D3D_ROOT_SIGNATURE_VERSION_1_1;
        rootSignatureDesc.Desc_1_1.NumParameters     = static_cast<UINT>(std::size(rootParameters));
        rootSignatureDesc.Desc_1_1.pParameters       = rootParameters;
        rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
        rootSignatureDesc.Desc_1_1.pStaticSamplers   = nullptr;
        
        // These two flags are what enable ResourceDescriptorHeap and SamplerDescriptorHeap
        // indexing in HLSL — the core of bindless
        rootSignatureDesc.Desc_1_1.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
            D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

        ID3DBlob* signatureBlob{ nullptr };
        ID3DBlob* errorBlob{ nullptr };
        
        if (FAILED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signatureBlob, &errorBlob)))
        {
            if (errorBlob != nullptr)
            {
                NL_FATAL(GCoreLogger, "root signature serialization failed: {}", static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            NL_FATAL(GCoreLogger, "failed to serialize root signature");
        }

        NL_DX_VERIFY(myDevice->CreateRootSignature(
            0,
            signatureBlob->GetBufferPointer(),
            signatureBlob->GetBufferSize(),
            IID_PPV_ARGS(&myRootSignature)));

        SafeRelease(signatureBlob);
        SafeRelease(errorBlob);

        N_D3D12_SET_NAME(myRootSignature, "Global Bindless Root Signature");
        NL_TRACE(GCoreLogger, "global root signature initialized [{} root constants]", ROOT_CONSTANT_COUNT);
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
        for (auto& pipeline : queue.pipelinesToDestroy)
        {
            SafeRelease(pipeline->pipelineState);
        }
        
        queue.pipelinesToDestroy.clear();
        queue.texturesToDestroy.clear();
        queue.buffersToDestroy.clear();
        queue.contextsToDestroy.clear(); // unique_ptr destructor handles cleanup
    }
}
