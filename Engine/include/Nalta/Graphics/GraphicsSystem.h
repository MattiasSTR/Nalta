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

    private:
        //std::unique_ptr<IDevice> myDevice;
        RHI::Device myDevice;
    };
}
