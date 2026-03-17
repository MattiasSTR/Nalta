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
        class Device;
        class RenderSurface;
    }
    
    class GraphicsSystem
    {
    public:
        GraphicsSystem();
        ~GraphicsSystem();
        
        void Initialize();
        void Shutdown();

        [[nodiscard]] Graphics::RenderSurfaceHandle CreateSurface(const Graphics::RenderSurfaceDesc& aDesc);
        void DestroySurface(Graphics::RenderSurfaceHandle aHandle);
        void DestroySurface(WindowHandle aWindow);

        void Present(Graphics::RenderSurfaceHandle aHandle) const;

        [[nodiscard]] Graphics::Device* GetDevice() const { return myDevice.get(); }

    private:
        struct SurfaceEntry
        {
            WindowHandle window;
            std::unique_ptr<Graphics::RenderSurface> surface;
        };

        std::unique_ptr<Graphics::Device>   myDevice;
        std::vector<SurfaceEntry>           mySurfaces;
    };
}
