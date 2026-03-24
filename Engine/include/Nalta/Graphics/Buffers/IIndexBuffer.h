#pragma once
#include "IBuffer.h"
#include "IGPUResource.h"

namespace Nalta::Graphics
{
    enum class IndexFormat : uint8_t
    {
        Uint16,
        Uint32
    };

    class IIndexBuffer : public IBuffer, public virtual IGPUResource
    {
    public:
        [[nodiscard]] BufferUsage GetUsage() const override { return BufferUsage::Index; }

        [[nodiscard]] virtual uint32_t GetIndexCount() const = 0;
        [[nodiscard]] virtual IndexFormat GetFormat() const = 0;
    };
}