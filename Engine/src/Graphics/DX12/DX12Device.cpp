#include "npch.h"
#include "Nalta/Graphics/DX12/DX12Device.h"

namespace Nalta::Graphics
{
    void DX12Device::Initialize()
    {
        NL_INFO(GCoreLogger, "[DX12Device] Initialize called");
    }

    void DX12Device::Shutdown()
    {
        NL_INFO(GCoreLogger, "[DX12Device] Shutdown called");
    }

    std::unique_ptr<Buffer> DX12Device::CreateBuffer(const BufferDesc& /*aDesc*/)
    {
        NL_INFO(GCoreLogger, "[DX12Device] CreateBuffer called");
        return nullptr;
    }

    std::shared_ptr<GraphicsContext> DX12Device::CreateGraphicsContext()
    {
        NL_INFO(GCoreLogger, "[DX12Device] CreateGraphicsContext called");
        return nullptr;
    }

    std::shared_ptr<ComputeContext> DX12Device::CreateComputeContext()
    {
        NL_INFO(GCoreLogger, "[DX12Device] CreateComputeContext called");
        return nullptr;
    }

    std::shared_ptr<UploadContext> DX12Device::CreateUploadContext()
    {
        NL_INFO(GCoreLogger, "[DX12Device] CreateUploadContext called");
        return nullptr;
    }

    void DX12Device::Present(const std::shared_ptr<RenderSurface> aSurface)
    {
        N_ASSERT(aSurface, "[DX12Device] Present called for nullptr surface");
        aSurface->Present();
    }
}