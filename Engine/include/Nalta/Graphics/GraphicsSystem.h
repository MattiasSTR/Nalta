#pragma once
#include "Nalta/RHI/Device.h"

#include <memory>

namespace Nalta::Graphics
{
    class IDevice;
    
    class GraphicsSystem
    {
    public:
        GraphicsSystem();
        ~GraphicsSystem();
        
        void Initialize();
        void Shutdown();
        
        void Test();

    private:
        std::unique_ptr<RHI::Device> myDevice;
    };
}
