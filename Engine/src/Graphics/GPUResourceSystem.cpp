#include "npch.h"

#include "Nalta/Graphics/GPUResourceSystem.h"

namespace Nalta::Graphics
{
    GPUResourceSystem::GPUResourceSystem() = default;
    GPUResourceSystem::~GPUResourceSystem() = default;

    void GPUResourceSystem::Initialize()
    {
        N_CORE_ASSERT(!myIsInitialized, "GpuResourceSystem already initialized");

        myDevice = std::make_unique<RHI::Device>();

        myIsInitialized = true;
    }

    void GPUResourceSystem::Shutdown()
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem was not initialized");
        
        myDevice->WaitForIdle();

        myRenderSurfaces.ForEach([this](std::unique_ptr<RHI::RenderSurface>& aSurface)
        {
            myDevice->DestroyRenderSurface(std::move(aSurface));
        });
        myRenderSurfaces = {};

        myTextures.ForEach([this](std::unique_ptr<RHI::Texture>& aTexture)
        {
            myDevice->DestroyTexture(std::move(aTexture));
        });
        myTextures = {};

        myBuffers.ForEach([this](std::unique_ptr<RHI::Buffer>& aBuffer)
        {
            myDevice->DestroyBuffer(std::move(aBuffer));
        });
        myBuffers = {};

        myDevice.reset();
        myIsInitialized = false;
    }

    class RHI::Device& GPUResourceSystem::GetDevice() const
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        return *myDevice;
    }

    TextureKey GPUResourceSystem::CreateTexture(const RHI::TextureCreationDesc& aDesc)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");

        std::unique_ptr<RHI::Texture> texture{ myDevice->CreateTexture(aDesc) };
        N_CORE_ASSERT(texture && texture->IsValid(), "Failed to create texture");

        return myTextures.Insert(std::move(texture));
    }

    TextureKey GPUResourceSystem::UploadTexture(const RHI::TextureUploadDesc& aDesc)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");

        std::unique_ptr<RHI::Texture> texture{ myDevice->CreateTexture(aDesc.desc) };
        N_CORE_ASSERT(texture && texture->IsValid(), "Failed to create texture for upload");

        RHI::UploadContext& uploadCtx{ myDevice->GetUploadContextForCurrentFrame() };
        uploadCtx.AddTextureUpload(texture.get(), aDesc);

        return myTextures.Insert(std::move(texture));
    }

    void GPUResourceSystem::DestroyTexture(const TextureKey aKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        N_CORE_ASSERT(myTextures.Contains(aKey), "TextureKey is not valid");

        myDevice->DestroyTexture(std::move(*myTextures.Get(aKey)));
        myTextures.Remove(aKey);
    }

    const RHI::Texture* GPUResourceSystem::GetTexture(const TextureKey aKey) const
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        return myTextures.Get(aKey)->get();
    }

    BufferKey GPUResourceSystem::CreateBuffer(const RHI::BufferCreationDesc& aDesc)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");

        std::unique_ptr<RHI::Buffer> buffer{ myDevice->CreateBuffer(aDesc) };
        N_CORE_ASSERT(buffer && buffer->IsValid(), "Failed to create buffer");

        return myBuffers.Insert(std::move(buffer));
    }

    BufferKey GPUResourceSystem::UploadBuffer(const RHI::BufferCreationDesc& aDesc, const RHI::BufferUploadDesc& aUploadDesc)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        N_CORE_ASSERT(aDesc.access == RHI::BufferAccessFlags::GpuOnly, "UploadBuffer is only for GpuOnly buffers - use CreateBuffer and write mapped data directly for CpuToGpu buffers");

        std::unique_ptr<RHI::Buffer> buffer{ myDevice->CreateBuffer(aDesc) };
        N_CORE_ASSERT(buffer && buffer->IsValid(), "Failed to create buffer for upload");

        RHI::UploadContext& uploadCtx{ myDevice->GetUploadContextForCurrentFrame() };
        uploadCtx.AddBufferUpload(buffer.get(), aUploadDesc);

        return myBuffers.Insert(std::move(buffer));
    }

    void GPUResourceSystem::DestroyBuffer(const BufferKey aKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        N_CORE_ASSERT(myBuffers.Contains(aKey), "BufferKey is not valid");

        myDevice->DestroyBuffer(std::move(*myBuffers.Get(aKey)));
        myBuffers.Remove(aKey);
    }

    const RHI::Buffer* GPUResourceSystem::GetBuffer(const BufferKey aKey) const
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");

        return myBuffers.Get(aKey)->get();
    }

    RenderSurfaceKey GPUResourceSystem::CreateRenderSurface(const RHI::RenderSurfaceDesc& aDesc)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");

        std::unique_ptr<RHI::RenderSurface> surface{ myDevice->CreateRenderSurface(aDesc) };
        N_CORE_ASSERT(surface != nullptr, "Failed to create render surface");

        return myRenderSurfaces.Insert(std::move(surface));
    }

    void GPUResourceSystem::DestroyRenderSurface(const RenderSurfaceKey aKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        N_CORE_ASSERT(myRenderSurfaces.Contains(aKey), "RenderSurfaceKey is not valid");

        myDevice->DestroyRenderSurface(std::move(*myRenderSurfaces.Get(aKey)));
        myRenderSurfaces.Remove(aKey);
    }

    RHI::RenderSurface* GPUResourceSystem::GetRenderSurface(const RenderSurfaceKey aKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        return myRenderSurfaces.Get(aKey)->get();
    }
}
