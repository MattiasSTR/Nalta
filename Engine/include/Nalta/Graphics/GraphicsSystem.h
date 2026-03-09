#pragma once
#include "Device.h"
#include "RenderSurface.h"

#include <memory>

namespace Nalta
{
    class GraphicsSystem
    {
    public:
        GraphicsSystem() = default;
        ~GraphicsSystem() = default;
        
        void Initialize();
        void Shutdown();
        
        std::shared_ptr<Graphics::RenderSurface> CreateRenderSurface(const std::shared_ptr<IWindow>& aWindow) const;
        void Present(const std::shared_ptr<Graphics::RenderSurface>& aSurface) const;
        
    private:
        std::unique_ptr<Graphics::Device> myDevice;
    };
}
