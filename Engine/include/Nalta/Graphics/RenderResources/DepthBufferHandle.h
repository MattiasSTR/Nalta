#pragma once

namespace Nalta::Graphics
{
    class IDepthBuffer;

    class DepthBufferHandle
    {
    public:
        DepthBufferHandle() = default;
        explicit DepthBufferHandle(IDepthBuffer* aBuffer) : myBuffer(aBuffer) {}

        [[nodiscard]] bool IsValid() const { return myBuffer != nullptr; }
        [[nodiscard]] IDepthBuffer* Get() const { return myBuffer; }

        IDepthBuffer* operator->() const { return myBuffer; }
        explicit operator bool() const { return IsValid(); }

        bool operator==(const DepthBufferHandle& aOther) const { return myBuffer == aOther.myBuffer; }
        bool operator!=(const DepthBufferHandle& aOther) const { return !(*this == aOther); }

    private:
        IDepthBuffer* myBuffer{ nullptr };
    };
}