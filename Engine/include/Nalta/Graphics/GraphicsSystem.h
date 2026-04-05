#pragma once
#include "Nalta/RHI/RHIDevice.h"

#include <memory>

namespace Nalta
{
    class WindowHandle;
}

namespace Nalta::Graphics
{
    class IDevice;
    
    class GraphicsSystem
    {
    public:
        GraphicsSystem();
        ~GraphicsSystem();
        
        void Initialize(const WindowHandle& aHandle);
        void Shutdown();
        
        void Test();

    private:
        std::unique_ptr<RHI::Device> myDevice;
    };
}
