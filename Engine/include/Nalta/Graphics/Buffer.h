#pragma once
#include <cstdint>

namespace Nalta::Graphics
{
    struct BufferDesc
    {
        uint32_t size{ 0 };
        uint32_t stride{ 0 };
        // usage flags, host-visible, etc.
    };
    
    class Buffer
    {
    public:
        explicit Buffer(const BufferDesc& aDesc) : myDesc(aDesc) {}
        ~Buffer() = default;
        
        [[nodiscard]] uint32_t GetSize() const { return myDesc.size; }
        [[nodiscard]] uint32_t GetStride() const { return myDesc.stride; }

    protected:
        BufferDesc myDesc;
    };
}