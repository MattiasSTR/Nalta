#pragma once

namespace Nalta
{
    class IWindow;
    struct WindowDesc;

    class IPlatformSystem
    {
    public:
        virtual ~IPlatformSystem() = default;

        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;

        virtual void PollEvents() = 0;

        virtual std::shared_ptr<IWindow> CreateWindow(const WindowDesc&) = 0;
    };
}