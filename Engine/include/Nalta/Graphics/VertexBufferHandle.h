#pragma once

namespace Nalta::Graphics
{
    class IVertexBuffer;

    class VertexBufferHandle
    {
    public:
        VertexBufferHandle() = default;
        explicit VertexBufferHandle(IVertexBuffer* aBuffer) : myBuffer(aBuffer) {}

        [[nodiscard]] bool IsValid() const { return myBuffer != nullptr; }
        [[nodiscard]] IVertexBuffer* Get() const { return myBuffer; }

        IVertexBuffer* operator->() const { return myBuffer; }
        explicit operator bool() const { return IsValid(); }

        bool operator==(const VertexBufferHandle& aOther) const { return myBuffer == aOther.myBuffer; }
        bool operator!=(const VertexBufferHandle& aOther) const { return !(*this == aOther); }

    private:
        IVertexBuffer* myBuffer{ nullptr };
    };
}