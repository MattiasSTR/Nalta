#include "npch.h"
#include "Nalta/Graphics/DX12/DX12Device.h"
#include "Nalta/Graphics/DX12/DX12Pipeline.h"
#include "Nalta/Graphics/PipelineDesc.h"
#include "Nalta/Graphics/Shader.h"
#include "Nalta/Graphics/DX12/DX12CopyQueue.h"
#include "Nalta/Graphics/DX12/DX12IndexBuffer.h"
#include "Nalta/Graphics/DX12/DX12RenderContext.h"
#include "Nalta/Graphics/DX12/DX12RenderSurface.h"
#include "Nalta/Graphics/DX12/DX12UploadBatch.h"
#include "Nalta/Graphics/DX12/DX12Util.h"
#include "Nalta/Graphics/DX12/DX12VertexBuffer.h"

#include <d3d12shader.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <objbase.h>
#include <objidl.h>
#include <wrl/client.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

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
        
        D3D12_CULL_MODE ToDX12CullMode(const CullMode aCullMode)
        {
            switch (aCullMode)
            {
                case CullMode::None:  return D3D12_CULL_MODE_NONE;
                case CullMode::Front: return D3D12_CULL_MODE_FRONT;
                case CullMode::Back:  return D3D12_CULL_MODE_BACK;
                default:               return D3D12_CULL_MODE_BACK;
            }
        }

        D3D12_FILL_MODE ToDX12FillMode(const FillMode aFillMode)
        {
            switch (aFillMode)
            {
                case FillMode::Solid:     return D3D12_FILL_MODE_SOLID;
                case FillMode::Wireframe: return D3D12_FILL_MODE_WIREFRAME;
                default:                  return D3D12_FILL_MODE_SOLID;
            }
        }
        
        struct ReflectedBinding
        {
            std::string             name;
            D3D_SHADER_INPUT_TYPE   type{};
            uint32_t                bindPoint{ 0 };
            uint32_t                bindCount{ 1 };
            uint32_t                space{ 0 };
            D3D12_SHADER_VISIBILITY visibility{ D3D12_SHADER_VISIBILITY_ALL };
        };
        
        struct ReflectedInputLayout
        {
            ComPtr<ID3D12ShaderReflection> reflection;
            std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
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
        
        ComPtr<ID3D12RootSignature> ReflectRootSignature(IDxcUtils* aDxcUtils, ID3D12Device10* aDevice, const Shader& aShader)
        {
            std::vector<ReflectedBinding> bindings;

            constexpr ShaderStage stages[]{ ShaderStage::Vertex, ShaderStage::Pixel, ShaderStage::Compute };
            for (const auto stage : stages)
            {
                if (!aShader.HasStage(stage)) continue;

                const auto& bytecode{ aShader.GetBytecode(stage) };
                if (!bytecode.HasReflection()) continue;

                const DxcBuffer reflBuffer
                {
                    .Ptr      = bytecode.GetReflectionData(),
                    .Size     = bytecode.GetReflectionSize(),
                    .Encoding = DXC_CP_ACP
                };

                ComPtr<ID3D12ShaderReflection> reflection;
                if (FAILED(aDxcUtils->CreateReflection(&reflBuffer, IID_PPV_ARGS(&reflection))))
                {
                    NL_WARN(GCoreLogger, "DX12Device: failed to reflect stage, skipping");
                    continue;
                }

                D3D12_SHADER_DESC shaderDesc{};
                reflection->GetDesc(&shaderDesc);

                const D3D12_SHADER_VISIBILITY visibility = [&]()
                {
                    switch (stage)
                    {
                        case ShaderStage::Vertex:  return D3D12_SHADER_VISIBILITY_VERTEX;
                        case ShaderStage::Pixel:   return D3D12_SHADER_VISIBILITY_PIXEL;
                        default:                   return D3D12_SHADER_VISIBILITY_ALL;
                    }
                }();
                
                for (UINT r{ 0 }; r < shaderDesc.BoundResources; ++r)
                {
                    D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
                    reflection->GetResourceBindingDesc(r, &bindDesc);

                    // If same resource used in multiple stages make it visible to all
                    auto existing{ std::ranges::find_if(bindings, [&](const ReflectedBinding& b)
                    {
                        return b.name == bindDesc.Name && b.space == bindDesc.Space;
                    }) };

                    if (existing != bindings.end())
                    {
                        existing->visibility = D3D12_SHADER_VISIBILITY_ALL;
                    }
                    else
                    {
                        bindings.push_back(
                       {
                           .name       = bindDesc.Name,
                           .type       = bindDesc.Type,
                           .bindPoint  = bindDesc.BindPoint,
                           .bindCount  = bindDesc.BindCount,
                           .space      = bindDesc.Space,
                           .visibility = visibility
                       });
                    }
                }
            }
            
            std::vector<D3D12_ROOT_PARAMETER1>     rootParams;
            std::vector<D3D12_DESCRIPTOR_RANGE1>   descriptorRanges;
            std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
            
            descriptorRanges.reserve(bindings.size());
            
            for (const auto& binding : bindings)
            {
                if (binding.type == D3D_SIT_SAMPLER)
                {
                    D3D12_STATIC_SAMPLER_DESC sampler{};
                    sampler.Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                    sampler.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                    sampler.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                    sampler.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                    sampler.ShaderRegister   = binding.bindPoint;
                    sampler.RegisterSpace    = binding.space;
                    sampler.ShaderVisibility = binding.visibility;
                    staticSamplers.push_back(sampler);
                    continue;
                }
            
                if (binding.type == D3D_SIT_CBUFFER)
                {
                    D3D12_ROOT_PARAMETER1 param{};
                    param.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
                    param.Descriptor.ShaderRegister = binding.bindPoint;
                    param.Descriptor.RegisterSpace  = binding.space;
                    param.Descriptor.Flags          = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
                    param.ShaderVisibility          = binding.visibility;
                    rootParams.push_back(param);
                    continue;
                }
                 
                const D3D12_DESCRIPTOR_RANGE_TYPE rangeType =
                    (binding.type == D3D_SIT_UAV_RWTYPED       ||
                     binding.type == D3D_SIT_UAV_RWSTRUCTURED  ||
                     binding.type == D3D_SIT_UAV_RWBYTEADDRESS)
                        ? D3D12_DESCRIPTOR_RANGE_TYPE_UAV
                        : D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            
                descriptorRanges.push_back(
                {
                    .RangeType                         = rangeType,
                    .NumDescriptors                    = binding.bindCount,
                    .BaseShaderRegister                = binding.bindPoint,
                    .RegisterSpace                     = binding.space,
                    .Flags                             = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
                    .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
                });
            
                D3D12_ROOT_PARAMETER1 param{};
                param.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                param.DescriptorTable.NumDescriptorRanges = 1;
                param.DescriptorTable.pDescriptorRanges   = &descriptorRanges.back();
                param.ShaderVisibility                    = binding.visibility;
                rootParams.push_back(param);
            }
            
            D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc{};
            rootSigDesc.Version                    = D3D_ROOT_SIGNATURE_VERSION_1_1;
            rootSigDesc.Desc_1_1.NumParameters     = static_cast<UINT>(rootParams.size());
            rootSigDesc.Desc_1_1.pParameters       = rootParams.empty() ? nullptr : rootParams.data();
            rootSigDesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
            rootSigDesc.Desc_1_1.pStaticSamplers   = staticSamplers.empty() ? nullptr : staticSamplers.data();
            rootSigDesc.Desc_1_1.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            
            ComPtr<ID3DBlob> serialized;
            ComPtr<ID3DBlob> serializeErrors;
            if (FAILED(D3D12SerializeVersionedRootSignature(&rootSigDesc, &serialized, &serializeErrors)))
            {
                if (serializeErrors)
                {
                    NL_ERROR(GCoreLogger, "DX12Device: root signature serialize error: {}", static_cast<const char*>(serializeErrors->GetBufferPointer()));
                }
                return nullptr;
            }
            
            ComPtr<ID3D12RootSignature> rootSignature;
            if (FAILED(aDevice->CreateRootSignature(
                0,
                serialized->GetBufferPointer(),
                serialized->GetBufferSize(),
                IID_PPV_ARGS(&rootSignature))))
            {
                NL_ERROR(GCoreLogger, "DX12Device: failed to create root signature");
                return nullptr;
            }

            NL_TRACE(GCoreLogger, "DX12Device: root signature reflected ({} params, {} samplers)", rootParams.size(), staticSamplers.size());

            return rootSignature;
        }
        
        ReflectedInputLayout ReflectInputLayout(IDxcUtils* aDxcUtils, const Shader& aShader)
        {
            ReflectedInputLayout result;

            if (!aShader.HasStage(ShaderStage::Vertex))
            {
                return result;
            }
            
            const auto& bytecode{ aShader.GetBytecode(ShaderStage::Vertex) };
            if (!bytecode.HasReflection())
            {
                return result;
            }
            
            const DxcBuffer reflBuffer
            {
                .Ptr      = bytecode.GetReflectionData(),
                .Size     = bytecode.GetReflectionSize(),
                .Encoding = DXC_CP_ACP
            };
            
            if (FAILED(aDxcUtils->CreateReflection(&reflBuffer, IID_PPV_ARGS(&result.reflection))))
            {
                NL_WARN(GCoreLogger, "DX12Device: failed to reflect vertex shader input layout");
                return result;
            }
            
            D3D12_SHADER_DESC shaderDesc{};
            result.reflection->GetDesc(&shaderDesc);
            
            for (UINT i{ 0 }; i < shaderDesc.InputParameters; ++i)
            {
                D3D12_SIGNATURE_PARAMETER_DESC paramDesc{};
                result.reflection->GetInputParameterDesc(i, &paramDesc);
            
                // Skip system value semantics like SV_VertexID, SV_InstanceID
                if (paramDesc.SystemValueType != D3D_NAME_UNDEFINED) continue;
            
                DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
            
                if (paramDesc.Mask == 0x1)
                {
                    if      (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32_FLOAT;
                    else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  format = DXGI_FORMAT_R32_SINT;
                    else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  format = DXGI_FORMAT_R32_UINT;
                }
                else if (paramDesc.Mask <= 0x3)
                {
                    if      (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32G32_FLOAT;
                    else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  format = DXGI_FORMAT_R32G32_SINT;
                    else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  format = DXGI_FORMAT_R32G32_UINT;
                }
                else if (paramDesc.Mask <= 0x7)
                {
                    if      (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32G32B32_FLOAT;
                    else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  format = DXGI_FORMAT_R32G32B32_SINT;
                    else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  format = DXGI_FORMAT_R32G32B32_UINT;
                }
                else if (paramDesc.Mask <= 0xF)
                {
                    if      (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                    else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  format = DXGI_FORMAT_R32G32B32A32_SINT;
                    else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  format = DXGI_FORMAT_R32G32B32A32_UINT;
                }
                
                result.elements.push_back(
                {
                    .SemanticName         = paramDesc.SemanticName,
                    .SemanticIndex        = paramDesc.SemanticIndex,
                    .Format               = format,
                    .InputSlot            = 0,
                    .AlignedByteOffset    = D3D12_APPEND_ALIGNED_ELEMENT,
                    .InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    .InstanceDataStepRate = 0
                });
            }
            
            NL_TRACE(GCoreLogger, "DX12Device: reflected {} input elements", result.elements.size());
            return result;
        }
    }
    
    struct DX12Device::Impl
    {
        ComPtr<IDXGIFactory7> factory;
        ComPtr<IDXGIAdapter4> adapter;
        ComPtr<ID3D12Device10> device;
        ComPtr<ID3D12CommandQueue> commandQueue;
        ComPtr<IDxcUtils> dxcUtils;
        
        DX12CopyQueue copyQueue;
        DX12UploadBatch uploadBatch;
        
        std::vector<ComPtr<ID3D12CommandAllocator>> commandAllocators;
        ComPtr<ID3D12GraphicsCommandList7> commandList;
        
        ComPtr<ID3D12Fence> fence;
        HANDLE fenceEvent{ nullptr };
        uint64_t fenceValue{ 0 };
        
        std::vector<uint64_t> frameFenceValues;
        uint32_t frameIndex{ 0 };
        uint32_t framesInFlight{ 2 };

#ifndef N_SHIPPING
        ComPtr<ID3D12Debug6> debugController;
        DWORD callbackCookie{ 0 };
#endif
    };
    
    DX12Device::DX12Device() = default;
    DX12Device::~DX12Device() = default;

    void DX12Device::Initialize(const DeviceDesc& aDesc)
    {
        myImpl = std::make_unique<Impl>();
        myImpl->framesInFlight = aDesc.framesInFlight;

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
        
        DX12_SET_NAME(myImpl->device.Get(), "Main D3D12 Device");

        InitInfoQueue();
        CreateCommandQueue();
        CreateCommandAllocators();
        CreateCommandList();
        CreateFence();
        
        myImpl->copyQueue.Initialize(myImpl->device.Get());
        myImpl->uploadBatch.Initialize(myImpl->device.Get(), &myImpl->copyQueue);
        NL_TRACE(GCoreLogger, "DX12Device: copy queue and upload batch initialized");
        
        if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&myImpl->dxcUtils))))
        {
            NL_FATAL(GCoreLogger, "DX12Device: failed to create DXC utils for reflection");
        }

        NL_INFO(GCoreLogger, "DX12Device: initialized");
    }

    void DX12Device::Shutdown()
    {
        SignalAndWait();

        if (myImpl->fenceEvent != nullptr)
        {
            CloseHandle(myImpl->fenceEvent);
            myImpl->fenceEvent = nullptr;
        }
        
#ifndef N_SHIPPING
        ComPtr<ID3D12DebugDevice> debugDevice;
        myImpl->device->QueryInterface(IID_PPV_ARGS(&debugDevice));
#endif
        
        myImpl->copyQueue.Shutdown();
        myImpl->uploadBatch.Shutdown();

        // Release everything before reporting
        myImpl->commandList.Reset();
        myImpl->commandAllocators.clear();
        myImpl->commandQueue.Reset();
        myImpl->fence.Reset();
        myImpl->dxcUtils.Reset();
        myImpl->adapter.Reset();
        myImpl->factory.Reset();
        myImpl->device.Reset();

#ifndef N_SHIPPING
        if (debugDevice != nullptr)
        {
            debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
            debugDevice.Reset();
        }
#endif
        
        myImpl.reset();
        NL_INFO(GCoreLogger, "DX12Device: shutdown");
    }

    void DX12Device::BeginFrame() const
    {
        const uint64_t completedValue{ myImpl->fence->GetCompletedValue() };
        if (myImpl->frameFenceValues[myImpl->frameIndex] > completedValue)
        {
            if (FAILED(myImpl->fence->SetEventOnCompletion(myImpl->frameFenceValues[myImpl->frameIndex], myImpl->fenceEvent)))
            {
                NL_FATAL(GCoreLogger, "DX12Device: failed to set fence event on BeginFrame");
            }
            WaitForSingleObject(myImpl->fenceEvent, INFINITE);
        }

        if (FAILED(myImpl->commandAllocators[myImpl->frameIndex]->Reset()))
        {
            NL_FATAL(GCoreLogger, "DX12Device: failed to reset command allocator");
        }

        if (FAILED(myImpl->commandList->Reset(myImpl->commandAllocators[myImpl->frameIndex].Get(), nullptr)))
        {
            NL_FATAL(GCoreLogger, "DX12Device: failed to reset command list");
        }
    }

    void DX12Device::EndFrame() const
    {
        if (FAILED(myImpl->commandList->Close()))
        {
            NL_FATAL(GCoreLogger, "DX12Device: failed to close command list");
        }

        ID3D12CommandList* lists[]{ myImpl->commandList.Get() };
        myImpl->commandQueue->ExecuteCommandLists(1, lists);

        Signal();
        myImpl->frameFenceValues[myImpl->frameIndex] = myImpl->fenceValue;
        myImpl->frameIndex = (myImpl->frameIndex + 1) % myImpl->framesInFlight;
    }

    void DX12Device::FlushUploads()
    {
        myImpl->uploadBatch.Flush();
    }

    std::unique_ptr<IVertexBuffer> DX12Device::CreateVertexBuffer(const VertexBufferDesc& aDesc, const std::span<const std::byte> aData)
    {
        auto buffer{ std::make_unique<DX12VertexBuffer>(aDesc.stride, aDesc.count) };
        myImpl->uploadBatch.QueueUpload(aData, buffer.get());

        NL_TRACE(GCoreLogger, "DX12Device: vertex buffer queued ({} vertices)", aDesc.count);
        return buffer;
    }

    std::unique_ptr<IIndexBuffer> DX12Device::CreateIndexBuffer(const IndexBufferDesc& aDesc, std::span<const std::byte> aData)
    {
        auto buffer{ std::make_unique<DX12IndexBuffer>(aDesc.count, aDesc.format) };
        myImpl->uploadBatch.QueueUpload(aData, buffer.get());
        NL_TRACE(GCoreLogger, "DX12Device: index buffer queued ({} indices)", aDesc.count);
        return buffer;
    }

    std::unique_ptr<IRenderSurface> DX12Device::CreateRenderSurface(const RenderSurfaceDesc& aDesc)
    {
        return std::make_unique<DX12RenderSurface>(aDesc, this);
    }

    std::unique_ptr<IPipeline> DX12Device::CreatePipeline(const PipelineDesc& aDesc)
    {
        N_CORE_ASSERT(aDesc.shader, "DX12Device: CreatePipeline called with null shader");

        ComPtr<ID3D12RootSignature> rootSignature{ ReflectRootSignature(myImpl->dxcUtils.Get(), myImpl->device.Get(), *aDesc.shader) };
        
        if (rootSignature == nullptr)
        {
            NL_ERROR(GCoreLogger, "DX12Device: failed to reflect root signature");
            return nullptr;
        }
        
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = rootSignature.Get();
        
        if (aDesc.shader->HasStage(ShaderStage::Vertex))
        {
            const auto& vs{ aDesc.shader->GetBytecode(ShaderStage::Vertex) };
            psoDesc.VS = { vs.GetData(), vs.GetSize() };
        }

        if (aDesc.shader->HasStage(ShaderStage::Pixel))
        {
            const auto& ps{ aDesc.shader->GetBytecode(ShaderStage::Pixel) };
            psoDesc.PS = { ps.GetData(), ps.GetSize() };
        }
        
        psoDesc.RasterizerState.FillMode              = ToDX12FillMode(aDesc.rasterizer.fillMode);
        psoDesc.RasterizerState.CullMode              = ToDX12CullMode(aDesc.rasterizer.cullMode);
        psoDesc.RasterizerState.FrontCounterClockwise = aDesc.rasterizer.frontCCW ? TRUE : FALSE;
        psoDesc.RasterizerState.DepthClipEnable       = TRUE;
        
        psoDesc.BlendState.AlphaToCoverageEnable  = FALSE;
        psoDesc.BlendState.IndependentBlendEnable = FALSE;
        for (auto& rt : psoDesc.BlendState.RenderTarget)
        {
            rt.BlendEnable           = aDesc.blend.blendEnabled ? TRUE : FALSE;
            rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        }
        
        psoDesc.DepthStencilState.DepthEnable    = aDesc.depth.depthEnabled ? TRUE : FALSE;
        psoDesc.DepthStencilState.DepthWriteMask = aDesc.depth.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        
        const auto reflectedLayout{ ReflectInputLayout(myImpl->dxcUtils.Get(), *aDesc.shader) };
        psoDesc.InputLayout =
        {
            .pInputElementDescs = reflectedLayout.elements.empty() ? nullptr : reflectedLayout.elements.data(),
            .NumElements        = static_cast<UINT>(reflectedLayout.elements.size())
        };
        
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets      = 1;
        psoDesc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat             = DXGI_FORMAT_UNKNOWN;
        psoDesc.SampleMask            = UINT_MAX;
        psoDesc.SampleDesc            = { 1, 0 };
        
        ComPtr<ID3D12PipelineState> pipelineState;
        if (FAILED(myImpl->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState))))
        {
            NL_ERROR(GCoreLogger, "DX12Device: failed to create pipeline state");
            return nullptr;
        }
        
        DX12_SET_NAME(rootSignature.Get(), "Reflected Root Signature");
        DX12_SET_NAME(pipelineState.Get(), "Graphics Pipeline State");

        NL_TRACE(GCoreLogger, "DX12Device: pipeline created");
        return std::make_unique<DX12Pipeline>(pipelineState.Get(), rootSignature.Get());
    }

    std::unique_ptr<IRenderContext> DX12Device::CreateRenderContext()
    {
        return std::make_unique<DX12RenderContext>(this);
    }

    void DX12Device::Signal() const
    {
        ++myImpl->fenceValue;
        if (FAILED(myImpl->commandQueue->Signal(myImpl->fence.Get(), myImpl->fenceValue)))
        {
            NL_FATAL(GCoreLogger, "DX12Device: failed to signal fence");
        }
    }

    void DX12Device::WaitForGPU() const
    {
        if (myImpl->fence->GetCompletedValue() < myImpl->fenceValue)
        {
            if (FAILED(myImpl->fence->SetEventOnCompletion(myImpl->fenceValue, myImpl->fenceEvent)))
            {
                NL_FATAL(GCoreLogger, "DX12Device: failed to set fence event");
            }

            WaitForSingleObject(myImpl->fenceEvent, INFINITE);
        }
    }

    void DX12Device::SignalAndWait() const
    {
        Signal();
        WaitForGPU();
    }

    DX12CopyQueue& DX12Device::GetCopyQueue() const
    {
        return myImpl->copyQueue;
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

    ID3D12GraphicsCommandList7* DX12Device::GetCommandList() const
    {
        return myImpl->commandList.Get();
    }

    uint64_t DX12Device::GetFenceValue() const
    {
        return myImpl->fenceValue;
    }

    uint64_t DX12Device::GetCompletedValue() const
    {
        return myImpl->fence->GetCompletedValue();
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
        
        // Message callback via ID3D12InfoQueue1 — routes to logger
        ComPtr<ID3D12InfoQueue1> infoQueue1;
        if (SUCCEEDED(myImpl->device->QueryInterface(IID_PPV_ARGS(&infoQueue1))))
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
                &myImpl->callbackCookie);
        }

        NL_TRACE(GCoreLogger, "DX12Device: info queue configured");
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

        auto toMB = [](SIZE_T bytes)
        {
            return static_cast<size_t>(bytes / (1024 * 1024));
        };
        
        char adapterName[128]{};
        WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, adapterName, sizeof(adapterName), nullptr, nullptr);
        NL_INFO(GCoreLogger, "DX12Device: selected adapter '{}' VRAM: {} MB", adapterName, toMB(desc.DedicatedVideoMemory));
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
        DX12_SET_NAME(myImpl->commandQueue.Get(), "Main Command Queue");
    }

    void DX12Device::CreateCommandAllocators() const
    {
        myImpl->commandAllocators.resize(myImpl->framesInFlight);
        myImpl->frameFenceValues.resize(myImpl->framesInFlight, 0);

        for (uint32_t i{ 0 }; i < myImpl->framesInFlight; ++i)
        {
            if (FAILED(myImpl->device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&myImpl->commandAllocators[i]))))
            {
                NL_FATAL(GCoreLogger, "DX12Device: failed to create command allocator {}", i);
            }
            
            const std::wstring name{ L"Command Allocator " + std::to_wstring(i) };
            DX12_SET_NAME_W(myImpl->commandAllocators[i].Get(), name.c_str());
        }

        NL_TRACE(GCoreLogger, "DX12Device: {} command allocators created", myImpl->framesInFlight);
    }

    void DX12Device::CreateCommandList() const
    {
        if (FAILED(myImpl->device->CreateCommandList1(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            D3D12_COMMAND_LIST_FLAG_NONE,
            IID_PPV_ARGS(&myImpl->commandList))))
        {
            NL_FATAL(GCoreLogger, "DX12Device: failed to create command list");
        }

        NL_TRACE(GCoreLogger, "DX12Device: command list created");
        DX12_SET_NAME(myImpl->commandList.Get(), "Main Command List");
    }

    void DX12Device::CreateFence() const
    {
        if (FAILED(myImpl->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&myImpl->fence))))
        {
            NL_FATAL(GCoreLogger, "DX12Device: failed to create fence");
        }

        myImpl->fenceValue = 0;
        myImpl->fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        if (myImpl->fenceEvent == nullptr)
        {
            NL_FATAL(GCoreLogger, "DX12Device: failed to create fence event");
        }

        NL_TRACE(GCoreLogger, "DX12Device: fence created");
        DX12_SET_NAME(myImpl->fence.Get(), "Main Fence");
    }
}
