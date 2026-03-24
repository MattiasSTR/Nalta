#pragma once
#include <cstdint>

namespace Nalta::Graphics
{
    class IGPUResource
    {
    public:
        virtual ~IGPUResource() = default;

        [[nodiscard]] virtual bool IsReady() const = 0;
        [[nodiscard]] virtual uint64_t GetGPUAddress() const = 0;
        [[nodiscard]] virtual uint32_t GetSizeInBytes() const = 0;
    };
}