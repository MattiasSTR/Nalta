#pragma once
#include "ITexture.h"

namespace Nalta::Graphics
{
    class TextureHandle
    {
    public:
        TextureHandle() = default;
        explicit TextureHandle(ITexture* aTexture) : myTexture(aTexture) {}

        [[nodiscard]] bool IsValid() const { return myTexture != nullptr; }
        [[nodiscard]] bool IsReady() const { return (myTexture != nullptr) && myTexture->IsReady(); }
        [[nodiscard]] ITexture* Get() const { return myTexture; }

        ITexture* operator->() const { return myTexture; }
        explicit operator bool() const { return IsValid(); }

        bool operator==(const TextureHandle& aOther) const { return myTexture == aOther.myTexture; }
        bool operator!=(const TextureHandle& aOther) const { return !(*this == aOther); }

    private:
        ITexture* myTexture{ nullptr };
    };
}
