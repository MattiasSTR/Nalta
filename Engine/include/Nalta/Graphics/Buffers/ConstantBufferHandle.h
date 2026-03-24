#pragma once

namespace Nalta::Graphics
{
    class IConstantBuffer;

    class ConstantBufferHandle
    {
    public:
        ConstantBufferHandle() = default;
        explicit ConstantBufferHandle(IConstantBuffer* aBuffer) : myBuffer(aBuffer) {}

        [[nodiscard]] bool IsValid() const { return myBuffer != nullptr; }
        [[nodiscard]] IConstantBuffer* Get() const { return myBuffer; }

        IConstantBuffer* operator->() const { return myBuffer; }
        explicit operator bool() const { return IsValid(); }

        bool operator==(const ConstantBufferHandle& aOther) const { return myBuffer == aOther.myBuffer; }
        bool operator!=(const ConstantBufferHandle& aOther) const { return !(*this == aOther); }

    private:
        IConstantBuffer* myBuffer{ nullptr };
    };
}