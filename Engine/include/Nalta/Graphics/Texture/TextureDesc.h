#pragma once
#include <cstdint>

namespace Nalta::Graphics
{
    enum class TextureFormat : uint8_t
    {
        Unknown,
        RGBA8_UNORM,        // standard color
        RGBA8_SRGB,         // sRGB color
        BC1_UNORM,          // DXT1 — opaque or 1-bit alpha
        BC1_SRGB,
        BC3_UNORM,          // DXT5 — full alpha
        BC3_SRGB,
        BC4_UNORM,          // single channel (roughness, AO)
        BC5_UNORM,          // two channel (normal maps RG)
        BC7_UNORM,          // high quality color + alpha
        BC7_SRGB,
        BC6H_UF16,          // HDR
    };

    struct TextureMipDesc
    {
        uint32_t rowPitch{ 0 };
        uint32_t slicePitch{ 0 };
    };
    
    struct TextureDesc
    {
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        uint32_t mipLevels{ 1 };
        TextureFormat format{ TextureFormat::RGBA8_UNORM };
        std::vector<TextureMipDesc> mips;
    };
}