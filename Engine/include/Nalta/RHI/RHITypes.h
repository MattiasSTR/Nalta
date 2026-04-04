#pragma once
#include <span>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace Nalta::RHI
{
    constexpr uint32_t FRAMES_IN_FLIGHT{ 2 };
    constexpr uint32_t BACK_BUFFER_COUNT{ 3 };
    
    // How many 32-bit root constants are available per draw
    // 16 uints = 64 bytes - enough for texture indices + per-object CBV address
    constexpr uint32_t ROOT_CONSTANT_COUNT{ 16 };
    
    enum class RootParameter : uint32_t
    {
        Constants = 0, // 32-bit push constants
        PassCBV = 1, // per-pass/per-frame constant buffer virtual address
    };
    
    enum class QueueType : uint8_t
    {
        Graphics = 0,
        Compute,
        Copy,
        Count
    };
    
    struct RenderSurfaceDesc
    {
        void* window{ nullptr };
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        uint32_t bufferCount{ BACK_BUFFER_COUNT };
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
    
    struct TextureCreationDesc
    {
        uint32_t width{ 1 };
        uint32_t height{ 1 };
        uint16_t depth{ 1 };
        uint16_t arraySize{ 1 };
        uint16_t mipLevels{ 1 };
        TextureFormat format{ TextureFormat::Unknown };
        TextureViewFlags viewFlags{ TextureViewFlags::None };
        std::string debugName{};

        [[nodiscard]] bool IsCubeMap()const { return arraySize == 6; }
        [[nodiscard]] bool HasFlag(const TextureViewFlags aFlag) const { return viewFlags & aFlag; }
    };
    
    struct TextureMipData
    {
        std::span<const std::byte> data{};
        uint32_t rowPitch{ 0 };
        uint32_t slicePitch{ 0 };
    };
    
    struct TextureUploadDesc
    {
        TextureCreationDesc desc{};
        std::vector<TextureMipData> mips{};
    };
    
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

    struct BufferCreationDesc
    {
        uint64_t size{ 0 };
        uint32_t stride{ 0 }; // 0 for raw/unstructured
        BufferAccessFlags access{ BufferAccessFlags::GpuOnly };
        BufferViewFlags viewFlags{ BufferViewFlags::None };
        bool isRawAccess{ false }; // ByteAddressBuffer in HLSL
        std::string debugName{};

        [[nodiscard]] bool HasFlag(const BufferViewFlags aFlag) const { return viewFlags & aFlag; }
    };
    
    enum class ShaderStage : uint8_t
    {
        Vertex,
        Pixel,
        Compute,
        Mesh,
        Amplification,
    };
    
    struct ShaderStageDesc
    {
        ShaderStage stage{};
        std::string entryPoint{};
    };

    struct ShaderDesc
    {
        std::filesystem::path filePath{};
        std::vector<ShaderStageDesc> stages{};
        std::unordered_map<std::string, std::string> defines{};
        std::string debugName{};
    };
    
    struct CompiledStage
    {
        std::vector<uint8_t> bytecode{};
        ShaderStage stage{};

        bool IsValid() const { return !bytecode.empty(); }
        const void* GetData() const { return bytecode.data(); }
        size_t GetSize() const { return bytecode.size(); }
    };
    
    struct Shader
    {
        std::vector<CompiledStage> stages{};
        std::string debugName{};

        const CompiledStage* GetStage(const ShaderStage aStage) const
        {
            for (const auto& s : stages)
            {
                if (s.stage == aStage)
                {
                    return &s;
                }
            }
            return nullptr;
        }

        bool HasStage(const ShaderStage aStage) const { return GetStage(aStage) != nullptr; }
        bool IsValid() const { return !stages.empty(); }
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

    struct RasterizerDesc
    {
        CullMode cullMode{ CullMode::Back };
        FillMode fillMode{ FillMode::Solid };
        bool frontCCW{ false };
        float depthBias{ 0.0f };
        float slopeScaledDepthBias{ 0.0f }; // needed for shadow maps
    };
    
    struct RenderTargetBlendDesc
    {
        bool enabled{ false };
        BlendFactor srcBlend{ BlendFactor::SrcAlpha };
        BlendFactor dstBlend{ BlendFactor::OneMinusSrcAlpha };
        BlendOp blendOp{ BlendOp::Add };
        BlendFactor srcBlendAlpha{ BlendFactor::One };
        BlendFactor dstBlendAlpha{ BlendFactor::Zero };
        BlendOp blendOpAlpha{ BlendOp::Add };
    };

    struct BlendDesc
    {
        // One per render target — deferred needs multiple targets
        std::array<RenderTargetBlendDesc, 8> renderTargets{};
    };
    
    struct DepthDesc
    {
        bool depthEnabled{ false };
        bool depthWrite{ true };
        DepthCompare compareFunc { DepthCompare::Greater  }; // reversed-Z default
        bool stencilEnabled{ false };
    };
    
    struct GraphicsPipelineDesc
    {
        const Shader* shader{ nullptr };
        RasterizerDesc rasterizer{};
        BlendDesc blend{};
        DepthDesc depth{};
        TextureFormat depthFormat{ TextureFormat::Unknown };
        std::vector<TextureFormat> renderTargetFormats{};
        PrimitiveTopology topology{ PrimitiveTopology::TriangleList };
        std::string debugName{};
    };

    struct ComputePipelineDesc
    {
        const Shader* shader{ nullptr };
        std::string debugName{};
    };
}