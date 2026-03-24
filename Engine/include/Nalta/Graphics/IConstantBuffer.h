#pragma once
#include "IBuffer.h"

namespace Nalta::Graphics
{
    class IConstantBuffer : public IBuffer
    {
    public:
        [[nodiscard]] BufferUsage GetUsage() const override { return BufferUsage::Constant; }
    };
}