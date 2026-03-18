#include "npch.h"
#include "Nalta/Graphics/GraphicsFactory.h"

#include "Nalta/Graphics/GraphicsConfig.h"

#ifdef N_GRAPHICS_API_DX12
#include "Nalta/Graphics/DX12/DX12Device.h"
#endif

namespace Nalta::Graphics
{
    std::unique_ptr<IDevice> CreateDevice()
    {
#ifdef N_GRAPHICS_API_DX12
        NL_INFO(GCoreLogger, "[GraphicsFactory] Creating DX12Device");
        return std::make_unique<DX12Device>();
#else
        static_assert(false, "No graphics API defined or API not supported");
#endif
    }
}