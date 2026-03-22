#pragma once
#include <cstdint>

namespace Nalta::Graphics
{
    enum class BufferUsage : uint8_t
    {
        Vertex,
        Index,
        Constant
    };

    class IBuffer
    {
    public:
        virtual ~IBuffer() = default;

        [[nodiscard]] virtual uint32_t GetSizeInBytes() const = 0;
        [[nodiscard]] virtual BufferUsage GetUsage() const = 0;
        [[nodiscard]] virtual bool IsValid() const = 0;
    };
}