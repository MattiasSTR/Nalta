#pragma once
#include "Device.h"

namespace Nalta::Graphics
{
    std::unique_ptr<Device> CreateDevice();
    std::shared_ptr<RenderSurface> CreateRenderSurface(const std::shared_ptr<IWindow>& aWindow);
}
