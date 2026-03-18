#include "npch.h"
#include "Nalta/Graphics/DX12/DX12Device.h"

#include "Nalta/Graphics/DX12/DX12RenderSurface.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

using Microsoft::WRL::ComPtr;

namespace Nalta::Graphics
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
        
        D3D_FEATURE_LEVEL QueryMaxFeatureLevel(IDXGIAdapter4* aAdapter)
        {
            // Temporary device just for querying feature level support
            ComPtr<ID3D12Device> tempDevice;
            if (FAILED(D3D12CreateDevice(aAdapter, MIN_FEATURE_LEVEL, IID_PPV_ARGS(&tempDevice))))
            {
                NL_FATAL(GCoreLogger, "DX12Device: failed to create temp device for feature level query");
            }

            D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelInfo{};
            featureLevelInfo.NumFeatureLevels        = static_cast<UINT>(std::size(FEATURE_LEVELS));
            featureLevelInfo.pFeatureLevelsRequested = FEATURE_LEVELS;

            if (FAILED(tempDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelInfo, sizeof(featureLevelInfo))))
            {
                NL_FATAL(GCoreLogger, "DX12Device: failed to query feature levels");
            }

            return featureLevelInfo.MaxSupportedFeatureLevel;
        }
    }
    
    struct DX12Device::Impl
    {
        ComPtr<IDXGIFactory7> factory;
        ComPtr<IDXGIAdapter4> adapter;
        ComPtr<ID3D12Device10> device;
        ComPtr<ID3D12CommandQueue> commandQueue;

#ifndef N_SHIPPING
        ComPtr<ID3D12Debug6> debugController;
#endif
    };
    
    DX12Device::DX12Device() = default;
    DX12Device::~DX12Device() = default;

    void DX12Device::Initialize()
    {
        myImpl = std::make_unique<Impl>();

        InitDebugLayer();

        UINT factoryFlags{ 0 };
#ifndef N_SHIPPING
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
        if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&myImpl->factory))))
        {
            NL_FATAL(GCoreLogger, "DX12Device: failed to create DXGI factory");
        }

        SelectAdapter();

        const D3D_FEATURE_LEVEL maxFeatureLevel{ QueryMaxFeatureLevel(myImpl->adapter.Get()) };
        NL_INFO(GCoreLogger, "DX12Device: max feature level {:#010x}", static_cast<uint32_t>(maxFeatureLevel));

        if (FAILED(D3D12CreateDevice(myImpl->adapter.Get(), maxFeatureLevel, IID_PPV_ARGS(&myImpl->device))))
        {
            NL_FATAL(GCoreLogger, "DX12Device: failed to create D3D12 device");
        }

        InitInfoQueue();
        CreateCommandQueue();

        NL_INFO(GCoreLogger, "DX12Device: initialized");
    }

    void DX12Device::Shutdown()
    {
        myImpl.reset();
        NL_INFO(GCoreLogger, "DX12Device: shutdown");
    }

    std::unique_ptr<Buffer> DX12Device::CreateBuffer(const BufferDesc& /*aDesc*/)
    {
        NL_INFO(GCoreLogger, "[DX12Device] CreateBuffer called");
        return nullptr;
    }

    std::unique_ptr<RenderSurface> DX12Device::CreateRenderSurface(const RenderSurfaceDesc& aDesc)
    {
        return std::make_unique<DX12RenderSurface>(aDesc, this);
    }

    std::shared_ptr<GraphicsContext> DX12Device::CreateGraphicsContext()
    {
        NL_INFO(GCoreLogger, "[DX12Device] CreateGraphicsContext called");
        return nullptr;
    }

    std::shared_ptr<ComputeContext> DX12Device::CreateComputeContext()
    {
        NL_INFO(GCoreLogger, "[DX12Device] CreateComputeContext called");
        return nullptr;
    }

    std::shared_ptr<UploadContext> DX12Device::CreateUploadContext()
    {
        NL_INFO(GCoreLogger, "[DX12Device] CreateUploadContext called");
        return nullptr;
    }

    void DX12Device::Present(RenderSurface* aSurface)
    {
        N_ASSERT(aSurface, "DX12Device: Present called with null surface");
        static_cast<DX12RenderSurface*>(aSurface)->Present();
    }

    IDXGIFactory7* DX12Device::GetFactory() const
    {
        return myImpl->factory.Get();
    }

    ID3D12Device10* DX12Device::GetDevice() const
    {
        return myImpl->device.Get();
    }

    ID3D12CommandQueue* DX12Device::GetCommandQueue() const
    {
        return myImpl->commandQueue.Get();
    }

    void DX12Device::InitDebugLayer() const
    {
#ifndef N_SHIPPING
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&myImpl->debugController))))
        {
            myImpl->debugController->EnableDebugLayer();
#ifdef N_DEBUG
            myImpl->debugController->SetEnableGPUBasedValidation(TRUE);
            NL_INFO(GCoreLogger, "DX12Device: GPU based validation is enabled. This will considerably slow down the renderer!");
#endif
            NL_TRACE(GCoreLogger, "DX12Device: debug layer enabled");
        }
        else
        {
            NL_WARN(GCoreLogger, "DX12Device: failed to enable debug layer");
        }
#endif
    }

    void DX12Device::InitInfoQueue() const
    {
#ifdef N_DEBUG
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(myImpl->device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
        {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
            NL_TRACE(GCoreLogger, "DX12Device: info queue configured");
        }
#endif
    }

    void DX12Device::SelectAdapter() const
    {
        IDXGIAdapter4* adapter{ nullptr };

        for (UINT i{ 0 }; myImpl->factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            // Pick first adapter that supports our minimum feature level
            if (SUCCEEDED(D3D12CreateDevice(adapter, MIN_FEATURE_LEVEL, __uuidof(ID3D12Device), nullptr)))
            {
                myImpl->adapter.Attach(adapter);
                break;
            }
            adapter->Release();
        }

        if (myImpl->adapter == nullptr)
        {
            NL_FATAL(GCoreLogger, "DX12Device: no adapter found that supports feature level 12_0");
        }

        DXGI_ADAPTER_DESC1 desc{};
        myImpl->adapter->GetDesc1(&desc);

        char adapterName[128]{};
        WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, adapterName, sizeof(adapterName), nullptr, nullptr);
        NL_INFO(GCoreLogger, "DX12Device: selected adapter '{}'", adapterName);
    }
    
    void DX12Device::CreateCommandQueue() const
    {
        D3D12_COMMAND_QUEUE_DESC desc{};
        desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;

        if (FAILED(myImpl->device->CreateCommandQueue(&desc, IID_PPV_ARGS(&myImpl->commandQueue))))
        {
            NL_FATAL(GCoreLogger, "DX12Device: failed to create command queue");
        }

        NL_TRACE(GCoreLogger, "DX12Device: command queue created");
    }
}
