#include "npch.h"
#include "Nalta/Graphics/DX12/DX12RenderSurface.h"

#include "Nalta/Platform/IWindow.h"

namespace Nalta::Graphics
{
    void DX12RenderSurface::Initialize(const std::shared_ptr<IWindow> aWindow)
    {
        myWindow = aWindow;
        NL_INFO(GCoreLogger, "[DX12RenderSurface] Initialized with window");
    }

    void DX12RenderSurface::Shutdown()
    {
        NL_INFO(GCoreLogger, "[DX12RenderSurface] Shutdown");
        myWindow.reset();
    }

    void DX12RenderSurface::Present()
    {
    }

    uint32_t DX12RenderSurface::GetWidth() const
    {
        N_ASSERT(myWindow, "[DX12RenderSurface] Window should not be nullptr");
        return myWindow->GetWidth();
    }

    uint32_t DX12RenderSurface::GetHeight() const
    {
        N_ASSERT(myWindow, "[DX12RenderSurface] Window should not be nullptr");
        return myWindow->GetHeight();
    }
}
