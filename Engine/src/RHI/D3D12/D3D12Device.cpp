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
    }

    void Device::EndFrame()
    {
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
                [](D3D12_MESSAGE_CATEGORY, D3D12_MESSAGE_SEVERITY aSeverity,
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

        auto toMB = [](SIZE_T bytes)
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
}
