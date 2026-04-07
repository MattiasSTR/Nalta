#pragma once
#include "Nalta/RHI/RHIDevice.h"
#include "Nalta/RHI/RHIContexts.h"
#include "Nalta/RHI/RHIResources.h"
#include "Nalta/RHI/Types/RHIDescs.h"
#include "Nalta/Graphics/GPUResourceKeys.h"
#include "Nalta/Platform/IWindow.h"
#include "Nalta/Util/SlotMap.h"

#include <variant>

namespace Nalta
{
    class IFileWatcher;
}

namespace Nalta::Graphics
{
    class GPUResourceManager
    {
    public:
        GPUResourceManager();
        ~GPUResourceManager();

        GPUResourceManager(const GPUResourceManager&) = delete;
        GPUResourceManager& operator=(const GPUResourceManager&) = delete;
        GPUResourceManager(GPUResourceManager&&) = delete;
        GPUResourceManager& operator=(GPUResourceManager&&) = delete;
        
        void Initialize(IFileWatcher* aFileWatcher = nullptr);
        void Shutdown();
        
        [[nodiscard]] RHI::Device& GetDevice() const;
        
        void OnShaderChanged(const std::filesystem::path& aPath);
        
        // Textures
        [[nodiscard]] TextureKey CreateTexture(const RHI::TextureCreationDesc& aDesc);
        [[nodiscard]] TextureKey UploadTexture(const RHI::TextureUploadDesc& aDesc);
        [[nodiscard]] bool IsTextureReady(TextureKey aKey) const;
        void DestroyTexture(TextureKey aKey);

        [[nodiscard]] const RHI::Texture* GetTexture(TextureKey aKey) const;
        
        // Buffers
        [[nodiscard]] BufferKey CreateBuffer(const RHI::BufferCreationDesc& aDesc);
        [[nodiscard]] BufferKey UploadBuffer(const RHI::BufferCreationDesc& aDesc, const RHI::BufferUploadDesc& aUploadDesc);
        [[nodiscard]] bool IsBufferReady(BufferKey aKey) const;
        void DestroyBuffer(BufferKey aKey);

        [[nodiscard]] const RHI::Buffer* GetBuffer(BufferKey aKey) const;
        
        // Render Surfaces
        [[nodiscard]] RenderSurfaceKey CreateRenderSurface(const RHI::RenderSurfaceDesc& aDesc, WindowKey aKey);
        void DestroyRenderSurface(RenderSurfaceKey aKey);
        void DestroyRenderSurface(WindowKey aKey);

        [[nodiscard]] RHI::RenderSurface* GetRenderSurface(RenderSurfaceKey aKey);
        
        // Shaders
        [[nodiscard]] ShaderKey CreateShader(const RHI::ShaderDesc& aDesc);
        void DestroyShader(ShaderKey aKey);
        
        [[nodiscard]] const RHI::Shader* GetShader(ShaderKey aKey) const;
        
        // Pipelines
        [[nodiscard]] PipelineKey CreateGraphicsPipeline(RHI::GraphicsPipelineDesc& aDesc, ShaderKey aShaderKey);
        [[nodiscard]] PipelineKey CreateComputePipeline(RHI::ComputePipelineDesc& aDesc, ShaderKey aShaderKey);
        [[nodiscard]] PipelineKey CreateGraphicsPipeline(const RHI::ShaderDesc& aShaderDesc, RHI::GraphicsPipelineDesc& aPipelineDesc);
        [[nodiscard]] PipelineKey CreateComputePipeline(const RHI::ShaderDesc& aShaderDesc, RHI::ComputePipelineDesc& aPipelineDesc);
        void DestroyPipeline(PipelineKey aKey);
        [[nodiscard]] const RHI::Pipeline* GetPipeline(PipelineKey aKey) const;
        
    private:
        std::unique_ptr<RHI::Device> myDevice;
        bool myIsInitialized{ false };
        
        IFileWatcher* myFileWatcher{ nullptr };
        
        SlotMap<TextureKey, std::unique_ptr<RHI::Texture>> myTextures;
        SlotMap<BufferKey, std::unique_ptr<RHI::Buffer>> myBuffers;
        
        SlotMap<RenderSurfaceKey, std::unique_ptr<RHI::RenderSurface>> myRenderSurfaces;
        std::unordered_map<WindowKey, RenderSurfaceKey> myWindowSurfaces;
        
        SlotMap<ShaderKey, std::unique_ptr<RHI::Shader>> myShaders;
        std::unordered_map<uint64_t, ShaderKey> myShaderCache;
        
        SlotMap<PipelineKey, std::unique_ptr<RHI::Pipeline>> myPipelines;
        std::unordered_map<uint64_t, PipelineKey> myPipelineCache;
        
        // Hot reload
        struct CachedPipelineEntry
        {
            ShaderKey shaderKey;
            std::variant<RHI::GraphicsPipelineDesc, RHI::ComputePipelineDesc> desc;
        };

        std::unordered_map<ShaderKey, RHI::ShaderDesc> myShaderDescs;
        std::unordered_map<ShaderKey, std::vector<PipelineKey>> myShaderPipelineDependents;
        std::unordered_map<PipelineKey, CachedPipelineEntry> myCachedPipelineEntries;
        std::unordered_map<std::string, std::vector<ShaderKey>> myPathToShaders;
    };
}
