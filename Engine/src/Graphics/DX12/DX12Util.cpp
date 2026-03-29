#include "npch.h"
#include "Nalta/Graphics/DX12/DX12Util.h"

namespace Nalta::Graphics
{
    DXGI_FORMAT ToDXGIFormat(const TextureFormat aFormat)
    {
        switch (aFormat)
        {
            case TextureFormat::RGBA8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::RGBA8_SRGB:  return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case TextureFormat::BC1_UNORM:   return DXGI_FORMAT_BC1_UNORM;
            case TextureFormat::BC1_SRGB:    return DXGI_FORMAT_BC1_UNORM_SRGB;
            case TextureFormat::BC3_UNORM:   return DXGI_FORMAT_BC3_UNORM;
            case TextureFormat::BC3_SRGB:    return DXGI_FORMAT_BC3_UNORM_SRGB;
            case TextureFormat::BC4_UNORM:   return DXGI_FORMAT_BC4_UNORM;
            case TextureFormat::BC5_UNORM:   return DXGI_FORMAT_BC5_UNORM;
            case TextureFormat::BC7_UNORM:   return DXGI_FORMAT_BC7_UNORM;
            case TextureFormat::BC7_SRGB:    return DXGI_FORMAT_BC7_UNORM_SRGB;
            case TextureFormat::BC6H_UF16:   return DXGI_FORMAT_BC6H_UF16;
            default:                          return DXGI_FORMAT_R8G8B8A8_UNORM;
        }
    }
}
