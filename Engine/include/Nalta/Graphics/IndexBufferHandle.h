#pragma once

namespace Nalta::Graphics
{
    class IIndexBuffer;

    class IndexBufferHandle
    {
    public:
        IndexBufferHandle() = default;
        explicit IndexBufferHandle(IIndexBuffer* aBuffer) : myBuffer(aBuffer) {}

        [[nodiscard]] bool IsValid() const { return myBuffer != nullptr; }
        [[nodiscard]] IIndexBuffer* Get() const { return myBuffer; }

        IIndexBuffer* operator->() const { return myBuffer; }
        explicit operator bool() const { return IsValid(); }

        bool operator==(const IndexBufferHandle& aOther) const { return myBuffer == aOther.myBuffer; }
        bool operator!=(const IndexBufferHandle& aOther) const { return !(*this == aOther); }

    private:
        IIndexBuffer* myBuffer{ nullptr };
    };
}