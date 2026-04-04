#pragma once
#include "D3D12Buffer.h"
#include "D3D12GraphicsContext.h"
#include "D3D12RenderSurface.h"
#include "D3D12UploadContext.h"
#include "Nalta/RHI/D3D12/D3D12Common.h"
#include "Nalta/RHI/D3D12/D3D12Context.h"
#include "Nalta/RHI/D3D12/D3D12Descriptor.h"
#include "Nalta/RHI/D3D12/D3D12Queue.h"
#include "Nalta/RHI/D3D12/D3D12Texture.h"

#include <array>
#include <D3D12MemAlloc.h>

namespace Nalta::RHI::D3D12
{
    namespace HeapCapacity
    {
        constexpr uint32_t BINDLESS{ 1'000'000 }; // CBV/SRV/UAV, shader-visible
        constexpr uint32_t SAMPLER{ 2'048 }; // shader-visible
        constexpr uint32_t RTV{ 512 }; // staging, non-shader-visible
        constexpr uint32_t DSV{ 128 }; // staging, non-shader-visible
    }
    
    class Device final
    {
    public:
        Device();
        ~Device();

        // Non-copyable, non-movable. There is one device.
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;
        Device(Device&&) = delete;
        Device& operator=(Device&&) = delete;
        
        void BeginFrame();
        void PrePresent();
        void EndFrame();
        
        ContextSubmissionResult SubmitContextWork(Context& aContext);
        void WaitOnContextWork(ContextSubmissionResult aSubmission, ContextWaitType aWaitType);
        void WaitForIdle();
        
        std::unique_ptr<GraphicsContext> CreateGraphicsContext();
        UploadContext& GetUploadContextForCurrentFrame() { return *myUploadContexts[myFrameIndex]; }
        
        std::unique_ptr<RenderSurface> CreateRenderSurface(const RenderSurfaceDesc& aDesc);
        std::unique_ptr<TextureResource> CreateTexture(const TextureCreationDesc& aDesc);
        std::unique_ptr<BufferResource> CreateBuffer(const BufferCreationDesc& aDesc);
        
        void DestroyRenderSurface(std::unique_ptr<RenderSurface> aSurface);
        void DestroyTexture(std::unique_ptr<TextureResource> aTexture);
        void DestroyBuffer(std::unique_ptr<BufferResource> aBuffer);
        void DestroyContext(std::unique_ptr<Context> aContext);
        
        [[nodiscard]] ID3D12Device10* GetD3D12Device() const { return myDevice; }
        [[nodiscard]] IDXGIFactory7* GetDXGIFactory() const { return myFactory; }
        [[nodiscard]] IDxcUtils* GetDxcUtils() const { return myDxcUtils; }
        [[nodiscard]] D3D12MA::Allocator* GetAllocator() const { return myAllocator; }
        [[nodiscard]] Queue& GetQueue(QueueType aType) { return *myQueues[static_cast<size_t>(aType)]; }
        [[nodiscard]] uint32_t& GetFrameIndex() { return myFrameIndex; }
        [[nodiscard]] FreeListDescriptorHeap& GetBindlessHeap() { return *myBindlessHeap; }
        [[nodiscard]] FreeListDescriptorHeap& GetSamplerHeap() { return *mySamplerHeap; }
        [[nodiscard]] FreeListDescriptorHeap& GetRTVHeap() { return *myRTVHeap; }
        [[nodiscard]] FreeListDescriptorHeap& GetDSVHeap() { return *myDSVHeap; }
        
    private:
        void InitDebugLayer();
        void InitInfoQueue();
        void SelectAdapter();
        void InitAllocator();
        void InitDescriptorHeaps();
        void CheckFeatureSupport() const;
        
        void ProcessDestructions(uint32_t aFrameIndex);
        
        IDXGIFactory7* myFactory{ nullptr };
        IDXGIAdapter4* myAdapter{ nullptr };
        ID3D12Device10* myDevice{ nullptr };
        IDxcUtils* myDxcUtils{ nullptr };
        D3D12MA::Allocator* myAllocator{ nullptr };
        
#ifndef N_SHIPPING
        ID3D12Debug6* myDebugController{ nullptr };
        DWORD myCallbackCookie{ 0 };
#endif
        
        std::array<std::unique_ptr<Queue>, static_cast<size_t>(QueueType::Count)> myQueues;
        
        std::unique_ptr<FreeListDescriptorHeap> myBindlessHeap;
        std::unique_ptr<FreeListDescriptorHeap> mySamplerHeap;
        std::unique_ptr<FreeListDescriptorHeap> myRTVHeap;
        std::unique_ptr<FreeListDescriptorHeap> myDSVHeap;
        
        std::array<std::unique_ptr<UploadContext>, FRAMES_IN_FLIGHT> myUploadContexts;
        
        uint32_t myFrameIndex{ 0 };
        
        struct FrameFences
        {
            uint64_t graphicsFence{ 0 };
            uint64_t computeFence{ 0 };
            uint64_t copyFence{ 0 };
        };
        std::array<FrameFences, FRAMES_IN_FLIGHT> myFrameFences{};
        
        struct DestructionQueue
        {
            std::vector<std::unique_ptr<TextureResource>> texturesToDestroy;
            std::vector<std::unique_ptr<BufferResource>> buffersToDestroy;
            std::vector<std::unique_ptr<Context>> contextsToDestroy;
        };
        std::array<DestructionQueue, FRAMES_IN_FLIGHT> myDestructionQueues;
        
        struct ContextSubmission
        {
            uint64_t fenceValue{ 0 };
            D3D12_COMMAND_LIST_TYPE type{};
        };
        std::array<std::vector<ContextSubmission>, FRAMES_IN_FLIGHT> myContextSubmissions;
    };
}
