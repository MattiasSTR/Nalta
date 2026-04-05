#pragma once
#include "Nalta/RHI/D3D12/Common/D3D12Common.h"
#include "Nalta/RHI/D3D12/Common/D3D12Descriptor.h"
#include "Nalta/RHI/D3D12/Common/D3D12Resource.h"
#include "Nalta/RHI/Types/RHIEnums.h"

namespace Nalta::RHI::D3D12
{
    struct TextureResource : Resource
    {
        TextureResource() : Resource()
        {
            type = ResourceType::Texture;
        }
        
        Descriptor SRVDescriptor{};
        Descriptor RTVDescriptor{};
        Descriptor DSVDescriptor{};
        Descriptor UAVDescriptor{};

        // The bindless heap index shaders use - only valid if SRV was created
        uint32_t GetBindlessIndex() const { return SRVDescriptor.heapIndex; }
        bool IsBackBuffer() const { return allocation == nullptr && resource != nullptr; }
    };
    
    constexpr DXGI_FORMAT TextureFormatToDXGI(const TextureFormat aFormat)
    {
        switch (aFormat)
        {
            case TextureFormat::RGBA8_UNORM:  return DXGI_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::RGBA8_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case TextureFormat::BGRA8_UNORM:  return DXGI_FORMAT_B8G8R8A8_UNORM;
            case TextureFormat::BGRA8_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            case TextureFormat::RGBA16_Float: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case TextureFormat::RGBA32_Float: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case TextureFormat::RG16_Float:   return DXGI_FORMAT_R16G16_FLOAT;
            case TextureFormat::RG32_Float:   return DXGI_FORMAT_R32G32_FLOAT;
            case TextureFormat::R16_Float:    return DXGI_FORMAT_R16_FLOAT;
            case TextureFormat::R32_Float:    return DXGI_FORMAT_R32_FLOAT;
            case TextureFormat::BC1_UNORM:    return DXGI_FORMAT_BC1_UNORM;
            case TextureFormat::BC1_SRGB:     return DXGI_FORMAT_BC1_UNORM_SRGB;
            case TextureFormat::BC3_UNORM:    return DXGI_FORMAT_BC3_UNORM;
            case TextureFormat::BC3_SRGB:     return DXGI_FORMAT_BC3_UNORM_SRGB;
            case TextureFormat::BC4_UNORM:    return DXGI_FORMAT_BC4_UNORM;
            case TextureFormat::BC5_UNORM:    return DXGI_FORMAT_BC5_UNORM;
            case TextureFormat::BC6H_UF16:    return DXGI_FORMAT_BC6H_UF16;
            case TextureFormat::BC7_UNORM:    return DXGI_FORMAT_BC7_UNORM;
            case TextureFormat::BC7_SRGB:     return DXGI_FORMAT_BC7_UNORM_SRGB;
            case TextureFormat::D32_Float:    return DXGI_FORMAT_D32_FLOAT;
            case TextureFormat::D24S8:        return DXGI_FORMAT_D24_UNORM_S8_UINT;
            case TextureFormat::D16_UNORM:    return DXGI_FORMAT_D16_UNORM;
            default:
                N_CORE_ASSERT(false, "Unknown TextureFormat");
                return DXGI_FORMAT_UNKNOWN;
        }
    }
    
    constexpr DXGI_FORMAT DepthFormatToTypeless(const TextureFormat aFormat)
    {
        switch (aFormat)
        {
            case TextureFormat::D32_Float: return DXGI_FORMAT_R32_TYPELESS;
            case TextureFormat::D24S8:     return DXGI_FORMAT_R24G8_TYPELESS;
            case TextureFormat::D16_UNORM: return DXGI_FORMAT_R16_TYPELESS;
            default:
                N_CORE_ASSERT(false, "Not a depth format");
                return DXGI_FORMAT_UNKNOWN;
        }
    }
    
    // SRV over a depth resource needs a float-readable format, not the depth format
    constexpr DXGI_FORMAT DepthFormatToSRV(const TextureFormat aFormat)
    {
        switch (aFormat)
        {
            case TextureFormat::D32_Float: return DXGI_FORMAT_R32_FLOAT;
            case TextureFormat::D24S8:     return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            case TextureFormat::D16_UNORM: return DXGI_FORMAT_R16_UNORM;
            default:
                N_CORE_ASSERT(false, "Not a depth format");
                return DXGI_FORMAT_UNKNOWN;
        }
    }
    
    inline bool IsDepthFormat(const TextureFormat aFormat)
    {
        return aFormat == TextureFormat::D32_Float || aFormat == TextureFormat::D24S8 || aFormat == TextureFormat::D16_UNORM;
    }
}
