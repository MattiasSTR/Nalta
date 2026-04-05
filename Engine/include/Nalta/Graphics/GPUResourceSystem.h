#pragma once
#include "Nalta/RHI/RHIDevice.h"
#include "Nalta/RHI/RHIContexts.h"
#include "Nalta/RHI/RHIResources.h"
#include "Nalta/RHI/Types/RHIDescs.h"
#include "Nalta/Graphics/GPUResourceKeys.h"
#include "Nalta/Util/SlotMap.h"

namespace Nalta::Graphics
{
    class GPUResourceSystem
    {
    public:
        GPUResourceSystem();
        ~GPUResourceSystem();

        GPUResourceSystem(const GPUResourceSystem&) = delete;
        GPUResourceSystem& operator=(const GPUResourceSystem&) = delete;
        GPUResourceSystem(GPUResourceSystem&&) = delete;
        GPUResourceSystem& operator=(GPUResourceSystem&&) = delete;
        
        void Initialize();
        void Shutdown();
        
        [[nodiscard]] RHI::Device& GetDevice() const;
        
        // Textures
        [[nodiscard]] TextureKey CreateTexture(const RHI::TextureCreationDesc& aDesc);
        [[nodiscard]] TextureKey UploadTexture(const RHI::TextureUploadDesc& aDesc);
        void DestroyTexture(TextureKey aKey);

        [[nodiscard]] const RHI::Texture* GetTexture(TextureKey aKey) const;
        
        // Buffers
        [[nodiscard]] BufferKey CreateBuffer(const RHI::BufferCreationDesc& aDesc);
        [[nodiscard]] BufferKey UploadBuffer(const RHI::BufferCreationDesc& aDesc, const RHI::BufferUploadDesc& aUploadDesc);
        void DestroyBuffer(BufferKey aKey);

        [[nodiscard]] const RHI::Buffer* GetBuffer(BufferKey aKey) const;
        
        // Render Surfaces
        [[nodiscard]] RenderSurfaceKey CreateRenderSurface(const RHI::RenderSurfaceDesc& aDesc);
        void DestroyRenderSurface(RenderSurfaceKey aKey);

        [[nodiscard]] RHI::RenderSurface* GetRenderSurface(RenderSurfaceKey aKey);
        
    private:
        std::unique_ptr<RHI::Device> myDevice;
        bool myIsInitialized{ false };
        
        // Store by value?
        SlotMap<TextureKey, std::unique_ptr<RHI::Texture>> myTextures;
        SlotMap<BufferKey, std::unique_ptr<RHI::Buffer>> myBuffers;
        SlotMap<RenderSurfaceKey, std::unique_ptr<RHI::RenderSurface>> myRenderSurfaces;
    };
}