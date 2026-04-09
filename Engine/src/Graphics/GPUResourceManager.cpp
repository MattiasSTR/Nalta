#include "npch.h"

#include "Nalta/Graphics/GPUResourceManager.h"

#include "Nalta/Graphics/ShaderBlobStore.h"
#include "Nalta/Platform/IFileWatcher.h"

namespace Nalta::Graphics
{
    namespace
    {
        uint64_t HashShaderDesc(const RHI::ShaderDesc& aDesc)
        {
            uint64_t hash{ std::hash<std::string>{}(aDesc.filePath.lexically_normal().string()) };
            for (const auto& stage : aDesc.stages)
            {
                hash ^= std::hash<std::string>{}(stage.entryPoint) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                hash ^= std::hash<int>{}(static_cast<int>(stage.stage))  + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            for (const auto& [key, value] : aDesc.defines)
            {
                hash ^= std::hash<std::string>{}(key)   + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                hash ^= std::hash<std::string>{}(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        }
        
        uint64_t HashGraphicsPipelineDesc(const RHI::GraphicsPipelineDesc& aDesc, const ShaderKey aShaderKey)
        {
            uint64_t hash{ std::hash<uint32_t>{}(aShaderKey.GetRaw()) };

            for (const auto& format : aDesc.renderTargetFormats)
            {
                hash ^= std::hash<int>{}(static_cast<int>(format)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }

            hash ^= std::hash<int>{}(static_cast<int>(aDesc.depthFormat)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<int>{}(static_cast<int>(aDesc.topology)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

            // Rasterizer
            hash ^= std::hash<int>{}(static_cast<int>(aDesc.rasterizer.cullMode)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<int>{}(static_cast<int>(aDesc.rasterizer.fillMode)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<bool>{}(aDesc.rasterizer.frontCCW) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

            // Depth
            hash ^= std::hash<bool>{}(aDesc.depth.depthEnabled) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<bool>{}(aDesc.depth.depthWrite) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<int>{}(static_cast<int>(aDesc.depth.compareFunc)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

            // Blend - only hash enabled targets
            for (const auto& rt : aDesc.blend.renderTargets)
            {
                hash ^= std::hash<bool>{}(rt.enabled) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                if (rt.enabled)
                {
                    hash ^= std::hash<int>{}(static_cast<int>(rt.srcBlend))    + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                    hash ^= std::hash<int>{}(static_cast<int>(rt.dstBlend))    + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                    hash ^= std::hash<int>{}(static_cast<int>(rt.blendOp))     + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                }
            }

            return hash;
        }
        
        uint64_t HashComputePipelineDesc(const ShaderKey aShaderKey)
        {
            return std::hash<SlotKey::RawType>{}(aShaderKey.GetRaw());
        }
    }
    
    GPUResourceManager::GPUResourceManager() = default;
    GPUResourceManager::~GPUResourceManager() = default;

    void GPUResourceManager::Initialize(IFileWatcher* aFileWatcher)
    {
        N_CORE_ASSERT(!myIsInitialized, "GpuResourceSystem already initialized");

        myDevice = std::make_unique<RHI::Device>();
        
#ifndef N_SHIPPING
        aFileWatcher->AddOnChangedCallback([this](const std::filesystem::path& aPath)
        {
            OnShaderChanged(aPath);
        });
#endif

        myIsInitialized = true;
    }

    void GPUResourceManager::Shutdown()
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem was not initialized");
        
        myDevice->WaitForIdle();
        
        myPipelines.ForEach([this](std::unique_ptr<RHI::Pipeline>& aPipeline)
        {
            myDevice->DestroyPipeline(std::move(aPipeline));
        });
        myPipelines = {};
        myPipelineCache.clear();

        myShaders.ForEach([](std::unique_ptr<RHI::Shader>& aShader)
        {
            aShader.reset();
        });
        myShaders = {};
        myShaderCache.clear();

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
        
        myShaderDescs.clear();
        myShaderPipelineDependents.clear();
        myCachedPipelineEntries.clear();
        myPathToShaders.clear();

        myDevice.reset();
        myIsInitialized = false;
    }

    class RHI::Device& GPUResourceManager::GetDevice() const
    {
        N_CORE_ASSERT(myIsInitialized, "GpuResourceSystem is not initialized");
        return *myDevice;
    }

    void GPUResourceManager::OnShaderChanged(const std::filesystem::path& aPath)
    {
        if (aPath.extension() != ".hlsl")
        {
            return;
        }
        
        const auto it{ myPathToShaders.find(aPath.lexically_normal().string()) };
        if (it == myPathToShaders.end())
        {
            NL_TRACE(GCoreLogger, "shader '{}' changed but is not loaded - skipping", aPath.string());
            return;
        }
    
        for (const ShaderKey shaderKey : it->second)
        {
            const RHI::ShaderDesc& desc{ myShaderDescs[shaderKey] };
    
            // Delete stale blob so next startup recompiles
            const uint64_t hash{ HashShaderDesc(desc) };
            const auto cachePath{ Paths::CookedDir() / "Shaders" / std::format("{:016x}.nshader", hash) };
            if (std::filesystem::exists(cachePath))
            {
                std::filesystem::remove(cachePath);
            }
    
            std::unique_ptr<RHI::Shader> newShader{ myDevice->CreateShader(desc) };
            if (!newShader || !newShader->IsValid())
            {
                NL_WARN(GCoreLogger, "shader recompile failed for '{}' - keeping previous version", aPath.string());
                continue;
            }
    
    
            ShaderBlobStore::Save(cachePath, *newShader, desc);
            
            std::vector<std::filesystem::path> oldIncludes{ myShaders.Get(shaderKey)->get()->includes };
            
            *myShaders.Get(shaderKey) = std::move(newShader);
            
            for (const auto& include : oldIncludes)
            {
                auto& vec{ myPathToShaders[include.string()] };
                std::erase(vec, shaderKey);
            }
            
            for (const auto& include : myShaders.Get(shaderKey)->get()->includes)
            {
                myPathToShaders[include.string()].push_back(shaderKey);
            }
    
            NL_INFO(GCoreLogger, "shader recompiled '{}'", desc.debugName);
            
            const auto depsIt{ myShaderPipelineDependents.find(shaderKey) };
            if (depsIt == myShaderPipelineDependents.end())
            {
                continue;
            }
    
            for (const PipelineKey pipelineKey : depsIt->second)
            {
                const auto entryIt{ myCachedPipelineEntries.find(pipelineKey) };
                if (entryIt == myCachedPipelineEntries.end())
                {
                    continue;
                }
    
                const RHI::Shader* shader{ GetShader(shaderKey) };
                std::unique_ptr<RHI::Pipeline> newPipeline{ nullptr };
    
                if (std::holds_alternative<RHI::GraphicsPipelineDesc>(entryIt->second.desc))
                {
                    RHI::GraphicsPipelineDesc pipelineDesc{ std::get<RHI::GraphicsPipelineDesc>(entryIt->second.desc) };
                    pipelineDesc.shader = shader;
                    newPipeline = myDevice->CreateGraphicsPipeline(pipelineDesc);
                }
                else
                {
                    RHI::ComputePipelineDesc pipelineDesc{ std::get<RHI::ComputePipelineDesc>(entryIt->second.desc) };
                    pipelineDesc.shader = shader;
                    newPipeline = myDevice->CreateComputePipeline(pipelineDesc);
                }
    
                if (!newPipeline)
                {
                    NL_WARN(GCoreLogger, "pipeline recompile failed - keeping previous version");
                    continue;
                }
    
                myDevice->DestroyPipeline(std::move(*myPipelines.Get(pipelineKey)));
                *myPipelines.Get(pipelineKey) = std::move(newPipeline);
                NL_INFO(GCoreLogger, "pipeline recompiled for shader '{}'", desc.debugName);
            }
        }
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

    ShaderKey GPUResourceManager::CreateShader(const RHI::ShaderDesc& aDesc)
    {
        N_CORE_ASSERT(myIsInitialized, "GPUResourceManager is not initialized");

        const uint64_t hash{ HashShaderDesc(aDesc) };
        if (const auto it{ myShaderCache.find(hash) }; it != myShaderCache.end())
        {
            return it->second;
        }

        const auto cachePath{ Paths::CookedDir() / "Shaders" / std::format("{:016x}.nshader", hash) };

        std::unique_ptr<RHI::Shader> shader;

#ifndef N_SHIPPING
        if (auto cached{ ShaderBlobStore::Load(cachePath, aDesc.filePath) })
        {
            NL_INFO(GCoreLogger, "loaded shader from cache '{}'", aDesc.debugName);
            shader = std::make_unique<RHI::Shader>(std::move(*cached));
        }
#endif

        if (!shader)
        {
            shader = myDevice->CreateShader(aDesc);
            N_CORE_ASSERT(shader && shader->IsValid(), "Failed to create shader");

#ifndef N_SHIPPING
            ShaderBlobStore::Save(cachePath, *shader, aDesc);
#endif
        }

        const ShaderKey key{ myShaders.Insert(std::move(shader)) };
        myShaderCache[hash] = key;
        myShaderDescs[key]  = aDesc;

        myPathToShaders[aDesc.filePath.lexically_normal().string()].push_back(key);
        
        for (const auto& include : myShaders.Get(key)->get()->includes)
        {
            myPathToShaders[include.string()].push_back(key);
        }

        return key;
    }

    void GPUResourceManager::DestroyShader(const ShaderKey aKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GPUResourceManager is not initialized");
        N_CORE_ASSERT(myShaders.Contains(aKey), "ShaderKey is not valid");

        for (auto it{ myShaderCache.begin() }; it != myShaderCache.end(); ++it)
        {
            if (it->second == aKey)
            {
                myShaderCache.erase(it);
                break;
            }
        }
        
        const auto& desc{ myShaderDescs[aKey] };
        
        auto& rootVec{ myPathToShaders[desc.filePath.lexically_normal().string()] };
        std::erase(rootVec, aKey);
        
        for (const auto& include : myShaders.Get(aKey)->get()->includes)
        {
            auto& vec{ myPathToShaders[include.string()] };
            std::erase(vec, aKey);
        }

        myShaderDescs.erase(aKey);
        myShaders.Remove(aKey);
    }

    const RHI::Shader* GPUResourceManager::GetShader(const ShaderKey aKey) const
    {
        N_CORE_ASSERT(myIsInitialized, "GPUResourceManager is not initialized");
        return myShaders.Get(aKey)->get();
    }

    PipelineKey GPUResourceManager::CreateGraphicsPipeline(RHI::GraphicsPipelineDesc& aDesc, const ShaderKey aShaderKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GPUResourceManager is not initialized");

        const uint64_t hash{ HashGraphicsPipelineDesc(aDesc, aShaderKey) };
        if (const auto it{ myPipelineCache.find(hash) }; it != myPipelineCache.end())
        {
            return it->second;
        }

        aDesc.shader = GetShader(aShaderKey);
        std::unique_ptr<RHI::Pipeline> pso{ myDevice->CreateGraphicsPipeline(aDesc) };
        N_CORE_ASSERT(pso != nullptr, "Failed to create graphics pipeline");

        const PipelineKey key{ myPipelines.Insert(std::move(pso)) };
        myPipelineCache[hash] = key;
        myShaderPipelineDependents[aShaderKey].push_back(key);
        myCachedPipelineEntries[key] = { aShaderKey, aDesc };
        return key;
    }

    PipelineKey GPUResourceManager::CreateComputePipeline(RHI::ComputePipelineDesc& aDesc, const ShaderKey aShaderKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GPUResourceManager is not initialized");

        const uint64_t hash{ HashComputePipelineDesc(aShaderKey) };
        if (const auto it{ myPipelineCache.find(hash) }; it != myPipelineCache.end())
        {
            return it->second;
        }

        aDesc.shader = GetShader(aShaderKey);
        std::unique_ptr<RHI::Pipeline> pso{ myDevice->CreateComputePipeline(aDesc) };
        N_CORE_ASSERT(pso != nullptr, "Failed to create compute pipeline");

        const PipelineKey key{ myPipelines.Insert(std::move(pso)) };
        myPipelineCache[hash] = key;
        myShaderPipelineDependents[aShaderKey].push_back(key);
        myCachedPipelineEntries[key] = { aShaderKey, aDesc };
        return key;
    }

    PipelineKey GPUResourceManager::CreateGraphicsPipeline(const RHI::ShaderDesc& aShaderDesc, RHI::GraphicsPipelineDesc& aPipelineDesc)
    {
        const ShaderKey shaderKey{ CreateShader(aShaderDesc) };
        return CreateGraphicsPipeline(aPipelineDesc, shaderKey);
    }

    PipelineKey GPUResourceManager::CreateComputePipeline(const RHI::ShaderDesc& aShaderDesc, RHI::ComputePipelineDesc& aPipelineDesc)
    {
        const ShaderKey shaderKey{ CreateShader(aShaderDesc) };
        return CreateComputePipeline(aPipelineDesc, shaderKey);
    }

    void GPUResourceManager::DestroyPipeline(const PipelineKey aKey)
    {
        N_CORE_ASSERT(myIsInitialized, "GPUResourceManager is not initialized");
        N_CORE_ASSERT(myPipelines.Contains(aKey), "PipelineKey is not valid");

        for (auto it{ myPipelineCache.begin() }; it != myPipelineCache.end(); ++it)
        {
            if (it->second == aKey)
            {
                myPipelineCache.erase(it);
                break;
            }
        }

        myDevice->DestroyPipeline(std::move(*myPipelines.Get(aKey)));
        myPipelines.Remove(aKey);
    }

    const RHI::Pipeline* GPUResourceManager::GetPipeline(const PipelineKey aKey) const
    {
        N_CORE_ASSERT(myIsInitialized, "GPUResourceManager is not initialized");
        return myPipelines.Get(aKey)->get();
    }
}
