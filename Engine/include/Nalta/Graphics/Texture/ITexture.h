#pragma once
#include "TextureDesc.h"

namespace Nalta::Graphics
{
    class ITexture
    {
    public:
        virtual ~ITexture() = default;

        [[nodiscard]] virtual bool IsValid()    const = 0;
        [[nodiscard]] virtual bool IsReady()    const = 0;
        [[nodiscard]] virtual uint32_t GetWidth()   const = 0;
        [[nodiscard]] virtual uint32_t GetHeight()  const = 0;
        [[nodiscard]] virtual uint32_t GetMipLevels() const = 0;
        [[nodiscard]] virtual TextureFormat GetFormat()  const = 0;
        [[nodiscard]] virtual uint32_t GetSRVIndex() const = 0; // index into SRV heap
    };
}