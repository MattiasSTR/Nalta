#pragma once
#include "IBuffer.h"
#include "IGPUResource.h"

namespace Nalta::Graphics
{
    class IVertexBuffer : public IBuffer, public virtual IGPUResource
    {
    public:
        [[nodiscard]] BufferUsage GetUsage() const override { return BufferUsage::Vertex; }

        [[nodiscard]] virtual uint32_t GetStride()      const = 0;
        [[nodiscard]] virtual uint32_t GetVertexCount() const = 0;
    };
}