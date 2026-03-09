#include "npch.h"
#include "Nalta/Graphics/GraphicsFactory.h"

#include "Nalta/Graphics/GraphicsConfig.h"

#ifdef N_GRAPHICS_API_DX12
#include "Nalta/Graphics/DX12/DX12Device.h"
#include "Nalta/Graphics/DX12/DX12RenderSurface.h"
#endif

namespace Nalta::Graphics
{
    std::unique_ptr<Device> CreateDevice()
    {
#ifdef N_GRAPHICS_API_DX12
        NL_INFO(GCoreLogger, "[GraphicsFactory] Creating DX12Device");
        return std::make_unique<DX12Device>();
#else
        static_assert(false, "No graphics API defined or API not supported");
#endif
    }

    std::shared_ptr<RenderSurface> CreateRenderSurface(const std::shared_ptr<IWindow>& aWindow)
    {
#ifdef N_GRAPHICS_API_DX12
        NL_INFO(GCoreLogger, "[GraphicsFactory] Creating DX12RenderSurface");
        auto surface{ std::make_shared<DX12RenderSurface>() };
        surface->Initialize(aWindow);
        return surface;
#else
        static_assert(false, "No graphics API defined or API not supported");
#endif
    }
}