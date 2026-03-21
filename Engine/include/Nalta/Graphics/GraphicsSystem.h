#pragma once
#include "PipelineDesc.h"
#include "PipelineHandle.h"
#include "RenderSurfaceDesc.h"
#include "RenderSurfaceHandle.h"
#include "ShaderCompiler.h"
#include "Nalta/Platform/WindowHandle.h"

#include <memory>
#include <vector>

namespace Nalta
{
    namespace Graphics
    {
        class IDevice;
        class IRenderSurface;
    }
    
    class GraphicsSystem
    {
    public:
        GraphicsSystem();
        ~GraphicsSystem();
        
        void Initialize();
        void Shutdown();
        
        void BeginFrame() const;
        void EndFrame() const;

        [[nodiscard]] Graphics::RenderSurfaceHandle CreateSurface(const Graphics::RenderSurfaceDesc& aDesc);
        void DestroySurface(Graphics::RenderSurfaceHandle aHandle);
        void DestroySurface(WindowHandle aWindow);
        
        [[nodiscard]] Graphics::PipelineHandle CreatePipeline(const Graphics::PipelineDesc& aDesc);
        void DestroyPipeline(Graphics::PipelineHandle aHandle);

        [[nodiscard]] Graphics::IDevice* GetDevice() const { return myDevice.get(); }
        [[nodiscard]] Graphics::ShaderCompiler& GetShaderCompiler() { return myShaderCompiler; }

    private:
        std::unique_ptr<Graphics::IDevice> myDevice;
        Graphics::ShaderCompiler myShaderCompiler;
        
        std::vector<std::unique_ptr<Graphics::IPipeline>> myPipelines;
        
        struct SurfaceEntry
        {
            WindowHandle window;
            std::unique_ptr<Graphics::IRenderSurface> surface;
        };
        std::vector<SurfaceEntry> mySurfaces;
    };
}
