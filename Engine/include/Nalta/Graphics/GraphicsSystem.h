#pragma once
#include "RenderSurfaceDesc.h"
#include "RenderSurfaceHandle.h"
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

        [[nodiscard]] Graphics::IDevice* GetDevice() const { return myDevice.get(); }

    private:
        struct SurfaceEntry
        {
            WindowHandle window;
            std::unique_ptr<Graphics::IRenderSurface> surface;
        };

        std::unique_ptr<Graphics::IDevice>   myDevice;
        std::vector<SurfaceEntry>           mySurfaces;
    };
}
