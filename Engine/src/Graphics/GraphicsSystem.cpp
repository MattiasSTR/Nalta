#include "npch.h"
#include "Nalta/Graphics/GraphicsSystem.h"

#include "Nalta/Graphics/GraphicsFactory.h"

namespace Nalta
{
    void GraphicsSystem::Initialize()
    {
        N_ASSERT(!myDevice && "GraphicsSystem already initialized");
        
        myDevice = Graphics::CreateDevice();
        if (myDevice)
        {
            myDevice->Initialize();
        }
    }
    
    void GraphicsSystem::Shutdown()
    {
        if (myDevice)
        {
            myDevice->Shutdown();
            myDevice.reset();
        }
    }

    std::shared_ptr<Graphics::RenderSurface> GraphicsSystem::CreateRenderSurface(const std::shared_ptr<IWindow>& aWindow) const
    {
        N_ASSERT(myDevice && "Device not initialized");
        return Graphics::CreateRenderSurface(aWindow);
    }

    void GraphicsSystem::Present(const std::shared_ptr<Graphics::RenderSurface>& aSurface) const
    {
        N_ASSERT(myDevice && aSurface && "Device or surface not initialized");
        myDevice->Present(aSurface);
    }
}
