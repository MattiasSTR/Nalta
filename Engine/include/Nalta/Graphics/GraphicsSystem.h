#pragma once

#include <memory>
#include <vector>
#include <span>

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
    };
}
