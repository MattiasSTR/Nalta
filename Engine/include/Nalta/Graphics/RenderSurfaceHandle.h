#pragma once

namespace Nalta::Graphics
{
    class RenderSurface;

    class RenderSurfaceHandle
    {
    public:
        RenderSurfaceHandle() = default;
        explicit RenderSurfaceHandle(RenderSurface* aSurface) : mySurface(aSurface) {}

        [[nodiscard]] bool IsValid() const { return mySurface != nullptr; }
        [[nodiscard]] RenderSurface* Get() const { return mySurface; }

        RenderSurface* operator->() const { return mySurface; }
        explicit operator bool()    const { return IsValid(); }

        bool operator==(const RenderSurfaceHandle& aOther) const { return mySurface == aOther.mySurface; }
        bool operator!=(const RenderSurfaceHandle& aOther) const { return !(*this == aOther); }

    private:
        RenderSurface* mySurface{ nullptr };
    };
}