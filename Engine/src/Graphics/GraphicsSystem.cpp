#include "npch.h"
#include "Nalta/Graphics/GraphicsSystem.h"
#include "Nalta/Platform/IWindow.h"

#ifdef N_RHI_D3D12
#include "Nalta/RHI/D3D12/D3D12Device.h"
#elif defined(N_RHI_VULKAN)
#include "Nalta/RHI/Vulkan/VulkanDevice.h"
#endif

namespace Nalta
{
    using namespace Graphics;

    GraphicsSystem::GraphicsSystem() = default;
    GraphicsSystem::~GraphicsSystem() = default;

    void GraphicsSystem::Initialize()
    {
        NL_SCOPE_CORE("GraphicsSystem");
        //N_CORE_ASSERT(myDevice == nullptr, "already initialized");

        myDevice = std::make_unique<RHI::Device>();
        
        NL_INFO(GCoreLogger, "initialized");
    }

    void GraphicsSystem::Shutdown()
    {
        NL_SCOPE_CORE("GraphicsSystem");
        
        NL_INFO(GCoreLogger, "shutdown");
    }
}
