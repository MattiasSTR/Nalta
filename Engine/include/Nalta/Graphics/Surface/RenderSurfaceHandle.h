#pragma once

namespace Nalta::Graphics
{
    class IRenderSurface;

    class RenderSurfaceHandle
    {
    public:
        RenderSurfaceHandle() = default;
        explicit RenderSurfaceHandle(IRenderSurface* aSurface) : mySurface(aSurface) {}

        [[nodiscard]] bool IsValid() const { return mySurface != nullptr; }
        [[nodiscard]] IRenderSurface* Get() const { return mySurface; }

        IRenderSurface* operator->() const { return mySurface; }
        explicit operator bool()    const { return IsValid(); }

        bool operator==(const RenderSurfaceHandle& aOther) const { return mySurface == aOther.mySurface; }
        bool operator!=(const RenderSurfaceHandle& aOther) const { return !(*this == aOther); }

    private:
        IRenderSurface* mySurface{ nullptr };
    };
}