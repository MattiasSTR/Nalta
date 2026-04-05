#pragma once
#include "RHIConstants.h"
#include "RHIEnums.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace Nalta::RHI
{
    struct Shader;

    struct RenderSurfaceDesc
    {
        void* window{ nullptr };
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        uint32_t bufferCount{ BACK_BUFFER_COUNT };
    };

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
    
    struct BufferUploadDesc
    {
        std::span<const std::byte> data{};
    };

    struct ShaderStageDesc
    {
        ShaderStage stage{};
        std::string entryPoint{};
    };

    struct RasterizerDesc
    {
        CullMode cullMode{ CullMode::Back };
        FillMode fillMode{ FillMode::Solid };
        bool frontCCW{ false };
        float depthBias{ 0.0f };
        float slopeScaledDepthBias{ 0.0f }; // for shadow maps
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
        // One per render target - deferred needs multiple targets
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
