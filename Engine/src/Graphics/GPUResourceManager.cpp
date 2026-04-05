#include "npch.h"

#include "Nalta/Graphics/GPUResourceManager.h"

namespace Nalta::Graphics
{
    GPUResourceManager::GPUResourceManager() = default;
    GPUResourceManager::~GPUResourceManager() = default;

    void GPUResourceManager::Initialize()
    {
        N_CORE_ASSERT(!myIsInitialized, "GpuResourceSystem already initialized");

        myDevice = std::make_unique<RHI::Device>();

        myIsInitialized = true;
    }

    void GPUResourceManager::Shutdown()
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

    class RHI::Device& GPUResourceManager::GetDevice() const
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        return *myDevice;
    }

    TextureKey GPUResourceManager::CreateTexture(const RHI::TextureCreationDesc& aDesc)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");

        std::unique_ptr<RHI::Texture> texture{ myDevice->CreateTexture(aDesc) };
        N_CORE_ASSERT(texture && texture->IsValid(), "Failed to create texture");

        return myTextures.Insert(std::move(texture));
    }

    TextureKey GPUResourceManager::UploadTexture(const RHI::TextureUploadDesc& aDesc)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");

        std::unique_ptr<RHI::Texture> texture{ myDevice->CreateTexture(aDesc.desc) };
        N_CORE_ASSERT(texture && texture->IsValid(), "Failed to create texture for upload");

        RHI::UploadContext& uploadCtx{ myDevice->GetUploadContextForCurrentFrame() };
        uploadCtx.AddTextureUpload(texture.get(), aDesc);

        return myTextures.Insert(std::move(texture));
    }

    bool GPUResourceManager::IsTextureReady(const TextureKey aKey) const
    {
        const auto* slot{ myTextures.Get(aKey) };
        if (slot == nullptr)
        {
            return false;
        }
        return (*slot)->isReady;
    }

    void GPUResourceManager::DestroyTexture(const TextureKey aKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        N_CORE_ASSERT(myTextures.Contains(aKey), "TextureKey is not valid");

        myDevice->DestroyTexture(std::move(*myTextures.Get(aKey)));
        myTextures.Remove(aKey);
    }

    const RHI::Texture* GPUResourceManager::GetTexture(const TextureKey aKey) const
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        return myTextures.Get(aKey)->get();
    }

    BufferKey GPUResourceManager::CreateBuffer(const RHI::BufferCreationDesc& aDesc)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");

        std::unique_ptr<RHI::Buffer> buffer{ myDevice->CreateBuffer(aDesc) };
        N_CORE_ASSERT(buffer && buffer->IsValid(), "Failed to create buffer");

        return myBuffers.Insert(std::move(buffer));
    }

    BufferKey GPUResourceManager::UploadBuffer(const RHI::BufferCreationDesc& aDesc, const RHI::BufferUploadDesc& aUploadDesc)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        N_CORE_ASSERT(aDesc.access == RHI::BufferAccessFlags::GpuOnly, "UploadBuffer is only for GpuOnly buffers - use CreateBuffer and write mapped data directly for CpuToGpu buffers");

        std::unique_ptr<RHI::Buffer> buffer{ myDevice->CreateBuffer(aDesc) };
        N_CORE_ASSERT(buffer && buffer->IsValid(), "Failed to create buffer for upload");

        RHI::UploadContext& uploadCtx{ myDevice->GetUploadContextForCurrentFrame() };
        uploadCtx.AddBufferUpload(buffer.get(), aUploadDesc);

        return myBuffers.Insert(std::move(buffer));
    }

    bool GPUResourceManager::IsBufferReady(const BufferKey aKey) const
    {
        const auto* slot{ myBuffers.Get(aKey) };
        if (slot == nullptr)
        {
            return false;
        }
        return (*slot)->isReady;
    }

    void GPUResourceManager::DestroyBuffer(const BufferKey aKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        N_CORE_ASSERT(myBuffers.Contains(aKey), "BufferKey is not valid");

        myDevice->DestroyBuffer(std::move(*myBuffers.Get(aKey)));
        myBuffers.Remove(aKey);
    }

    const RHI::Buffer* GPUResourceManager::GetBuffer(const BufferKey aKey) const
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");

        return myBuffers.Get(aKey)->get();
    }

    RenderSurfaceKey GPUResourceManager::CreateRenderSurface(const RHI::RenderSurfaceDesc& aDesc, const WindowKey aKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");

        std::unique_ptr<RHI::RenderSurface> surface{ myDevice->CreateRenderSurface(aDesc) };
        N_CORE_ASSERT(surface != nullptr, "Failed to create render surface");

        const RenderSurfaceKey key{ myRenderSurfaces.Insert(std::move(surface)) };
        myWindowSurfaces[aKey] = key;
        return key;
    }

    void GPUResourceManager::DestroyRenderSurface(const RenderSurfaceKey aKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        N_CORE_ASSERT(myRenderSurfaces.Contains(aKey), "RenderSurfaceKey is not valid");

        // Remove window mapping if one exists for this key
        for (auto it{ myWindowSurfaces.begin() }; it != myWindowSurfaces.end(); ++it)
        {
            if (it->second == aKey)
            {
                myWindowSurfaces.erase(it);
                break;
            }
        }

        myDevice->DestroyRenderSurface(std::move(*myRenderSurfaces.Get(aKey)));
        myRenderSurfaces.Remove(aKey);
    }

    void GPUResourceManager::DestroyRenderSurface(const WindowKey aKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");

        const auto it{ myWindowSurfaces.find(aKey) };
        N_CORE_ASSERT(it != myWindowSurfaces.end(), "No render surface found for window handle");

        DestroyRenderSurface(it->second);
    }

    RHI::RenderSurface* GPUResourceManager::GetRenderSurface(const RenderSurfaceKey aKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        return myRenderSurfaces.Get(aKey)->get();
    }
}
