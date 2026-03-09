#pragma once
#include <memory>
#include <cstdint>

namespace Nalta
{
    class IWindow;

    namespace Graphics
    {
        class RenderSurface
        {
        public:
            virtual ~RenderSurface() = default;
            
            virtual void Initialize(std::shared_ptr<IWindow> aWindow) = 0;

            virtual void Shutdown() = 0;

            virtual void Present() = 0;
            [[nodiscard]] virtual uint32_t GetWidth() const = 0;
            [[nodiscard]] virtual uint32_t GetHeight() const = 0;
        };
    }
}