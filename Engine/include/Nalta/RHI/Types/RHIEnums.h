#pragma once
#include <cstdint>

namespace Nalta::RHI
{
    enum class QueueType : uint8_t
    {
        Graphics = 0,
        Compute,
        Copy,
        Count
    };

    enum class ResourceType : uint8_t
    {
        Texture,
        Buffer,
    };

    enum class TextureFormat : uint8_t
    {
        Unknown,

        // LDR color
        RGBA8_UNORM,
        RGBA8_SRGB,
        BGRA8_UNORM,
        BGRA8_SRGB,

        // HDR color
        RGBA16_Float,
        RGBA32_Float,
        RG16_Float,
        RG32_Float,
        R16_Float,
        R32_Float,

        // Block compressed
        BC1_UNORM,
        BC1_SRGB,
        BC3_UNORM,
        BC3_SRGB,
        BC4_UNORM,
        BC5_UNORM,
        BC6H_UF16,
        BC7_UNORM,
        BC7_SRGB,

        // Depth
        D32_Float,
        D24S8,
        D16_UNORM,
    };
    
    enum class TextureViewFlags : uint32_t
    {
        None            = 0,
        ShaderResource  = 1 << 0, // SRV - sampled in shaders
        RenderTarget    = 1 << 1, // RTV - written as color target
        DepthStencil    = 1 << 2, // DSV - written as depth target
        UnorderedAccess = 1 << 3, // UAV - compute writes, ray tracing output
    };

    constexpr TextureViewFlags operator|(TextureViewFlags aA, TextureViewFlags aB)
    {
        return static_cast<TextureViewFlags>(static_cast<uint32_t>(aA) | static_cast<uint32_t>(aB));
    }

    constexpr TextureViewFlags& operator|=(TextureViewFlags& aA, TextureViewFlags aB)
    {
        aA = aA | aB;
        return aA;
    }

    constexpr bool operator&(TextureViewFlags aA, TextureViewFlags aB)
    {
        return (static_cast<uint32_t>(aA) & static_cast<uint32_t>(aB)) != 0u;
    }

    enum class BufferAccessFlags : uint8_t
    {
        GpuOnly,  // Default heap - fastest GPU access, updated via upload
        CpuToGpu, // Upload heap - written from CPU every frame, constant buffers, dynamic data
        GpuToCpu, // Readback heap - GPU writes, CPU reads, occlusion queries, stats
    };

    enum class BufferViewFlags : uint32_t
    {
        None            = 0,
        ConstantBuffer  = 1 << 0, // CBV - bound as constant buffer
        ShaderResource  = 1 << 1, // SRV - read in shaders as structured/raw buffer
        UnorderedAccess = 1 << 2, // UAV - read/write in compute, GPU-driven data
        AccelStructure  = 1 << 3, // ray tracing BLAS/TLAS
    };
    
    constexpr BufferViewFlags operator|(BufferViewFlags aA, BufferViewFlags aB)
    {
        return static_cast<BufferViewFlags>(static_cast<uint32_t>(aA) | static_cast<uint32_t>(aB));
    }

    constexpr BufferViewFlags& operator|=(BufferViewFlags& aA, BufferViewFlags aB)
    {
        aA = aA | aB;
        return aA;
    }

    constexpr bool operator&(BufferViewFlags aA, BufferViewFlags aB)
    {
        return (static_cast<uint32_t>(aA) & static_cast<uint32_t>(aB)) != 0u;
    }

    enum class ShaderStage : uint8_t
    {
        Vertex,
        Pixel,
        Compute,
        Mesh,
        Amplification,
    };
    
    enum class CullMode : uint8_t
    {
        None,
        Front,
        Back,
    };
    
    enum class FillMode : uint8_t
    {
        Solid,
        Wireframe,
    };

    enum class BlendFactor : uint8_t
    {
        Zero,
        One,
        SrcAlpha,
        OneMinusSrcAlpha,
        SrcColor,
        OneMinusSrcColor,
    };

    enum class BlendOp : uint8_t
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max,
    };
    
    enum class DepthCompare : uint8_t
    {
        Never,
        Less,
        LessEqual,
        Equal,
        GreaterEqual,   // reversed-Z
        Greater,        // reversed-Z
        NotEqual,
        Always,
    };

    enum class PrimitiveTopology : uint8_t
    {
        TriangleList,
        TriangleStrip,
        LineList,
        LineStrip,
        PointList,
    };
}