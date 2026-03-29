#include "npch.h"
#include "Nalta/Graphics/DX12/DX12Texture.h"

#include <d3d12.h>

namespace Nalta::Graphics
{
    DX12Texture::DX12Texture(const uint32_t aWidth, const uint32_t aHeight, const uint32_t aMipLevels, const TextureFormat aFormat, const uint32_t aSRVIndex)
        : myWidth(aWidth)
        , myHeight(aHeight)
        , myMipLevels(aMipLevels)
        , myFormat(aFormat)
        , mySRVIndex(aSRVIndex)
    {}

    void DX12Texture::SetResource(ID3D12Resource* aResource)
    {
        myResource = aResource;
    }

    uint64_t DX12Texture::GetGPUAddress() const
    {
        if (myResource == nullptr)
        {
            return 0;
        }
        
        return myResource->GetGPUVirtualAddress();
    }

    uint32_t DX12Texture::GetSizeInBytes() const
    {
        // Approximate — actual size depends on format and mip chain
        // This is just for debugging purposes
        return myWidth * myHeight * 4; // assumes RGBA8
    }

    void DX12Texture::OwnResource(Microsoft::WRL::ComPtr<ID3D12Resource> aResource)
    {
        myOwnedResource = std::move(aResource);
        myResource = myOwnedResource.Get();
    }
}
