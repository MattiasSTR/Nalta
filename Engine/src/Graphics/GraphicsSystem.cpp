#include "npch.h"
#include "Nalta/Graphics/GraphicsSystem.h"

#include "Nalta/Platform/IWindow.h"
#include "Nalta/Platform/WindowHandle.h"

#ifdef N_RHI_D3D12
#include "Nalta/RHI/D3D12/D3D12Device.h"
#elif defined(N_RHI_VULKAN)
#include "Nalta/RHI/Vulkan/VulkanDevice.h"
#endif

namespace Nalta
{
    using namespace Graphics;
    
    static std::unique_ptr<RHI::D3D12::GraphicsContext> context;
    static std::unique_ptr<RHI::D3D12::RenderSurface> surface;

    GraphicsSystem::GraphicsSystem() = default;
    GraphicsSystem::~GraphicsSystem() = default;

    void GraphicsSystem::Initialize(const WindowHandle& aHandle)
    {
        NL_SCOPE_CORE("GraphicsSystem");
        //N_CORE_ASSERT(myDevice == nullptr, "already initialized");
        
        RHI::RenderSurfaceDesc rdesc;
        rdesc.height = aHandle->GetHeight();
        rdesc.width = aHandle->GetWidth();
        rdesc.window = aHandle->GetNativeHandle();

        myDevice = std::make_unique<RHI::Device>();
        context = myDevice->CreateGraphicsContext();
        surface = myDevice->CreateRenderSurface(rdesc);
        
        auto MakeSolidColorPixels = [](uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        {
            constexpr uint32_t width     = 4;
            constexpr uint32_t height    = 4;
            constexpr uint32_t pixelSize = 4;
            constexpr uint32_t dataSize  = width * height * pixelSize;
        
            std::array<uint8_t, dataSize> pixels{};
            for (uint32_t i{ 0 }; i < dataSize; i += pixelSize)
            {
                pixels[i + 0] = r;
                pixels[i + 1] = g;
                pixels[i + 2] = b;
                pixels[i + 3] = a;
            }
            return pixels;
        };
        
        constexpr uint32_t width     = 4;
        constexpr uint32_t height    = 4;
        constexpr uint32_t pixelSize = 4;
        constexpr uint32_t rowPitch  = width * pixelSize;
        constexpr uint32_t dataSize  = rowPitch * height;
        
        // Create four textures
        Nalta::RHI::TextureCreationDesc desc{};
        desc.width     = width;
        desc.height    = height;
        desc.format    = Nalta::RHI::TextureFormat::RGBA8_UNORM;
        desc.viewFlags = Nalta::RHI::TextureViewFlags::ShaderResource;
        
        desc.debugName = "RedTexture";
        auto redTexture = myDevice->CreateTexture(desc);
        
        desc.debugName = "GreenTexture";
        auto greenTexture = myDevice->CreateTexture(desc);
        
        desc.debugName = "BlueTexture";
        auto blueTexture = myDevice->CreateTexture(desc);
        
        desc.debugName = "WhiteTexture";
        auto whiteTexture = myDevice->CreateTexture(desc);
        
        N_CORE_ASSERT(!redTexture->isReady,   "Should not be ready before upload");
        N_CORE_ASSERT(!greenTexture->isReady, "Should not be ready before upload");
        N_CORE_ASSERT(!blueTexture->isReady,  "Should not be ready before upload");
        N_CORE_ASSERT(!whiteTexture->isReady, "Should not be ready before upload");
        
        // Pixel data — kept alive until upload is processed
        auto redPixels   = MakeSolidColorPixels(255, 0,   0);
        auto greenPixels = MakeSolidColorPixels(0,   255, 0);
        auto bluePixels  = MakeSolidColorPixels(0,   0,   255);
        auto whitePixels = MakeSolidColorPixels(255, 255, 255);
        
        auto MakeUploadDesc = [&](const Nalta::RHI::TextureCreationDesc& aDesc,
                                   const auto& aPixels) -> Nalta::RHI::TextureUploadDesc
        {
            Nalta::RHI::TextureMipData mip{};
            mip.data       = std::as_bytes(std::span{ aPixels });
            mip.rowPitch   = rowPitch;
            mip.slicePitch = dataSize;
        
            Nalta::RHI::TextureUploadDesc upload{};
            upload.desc = aDesc;
            upload.mips.push_back(mip);
            return upload;
        };
        
        // Frame 1 — queue all uploads
        myDevice->BeginFrame();
        
        auto& uploadCtx = myDevice->GetUploadContextForCurrentFrame();
        
        desc.debugName = "RedTexture";
        uploadCtx.AddTextureUpload(redTexture.get(),   MakeUploadDesc(desc, redPixels));
        desc.debugName = "GreenTexture";
        uploadCtx.AddTextureUpload(greenTexture.get(), MakeUploadDesc(desc, greenPixels));
        desc.debugName = "BlueTexture";
        uploadCtx.AddTextureUpload(blueTexture.get(),  MakeUploadDesc(desc, bluePixels));
        desc.debugName = "WhiteTexture";
        uploadCtx.AddTextureUpload(whiteTexture.get(), MakeUploadDesc(desc, whitePixels));
        
        myDevice->PrePresent();
        myDevice->EndFrame();
        
        // Frame 2 — in flight
        myDevice->BeginFrame();
        myDevice->PrePresent();
        myDevice->EndFrame();
        
        // Frame 3 — resolved
        myDevice->BeginFrame();
        
        N_CORE_ASSERT(redTexture->isReady,   "Red texture should be ready");
        N_CORE_ASSERT(greenTexture->isReady, "Green texture should be ready");
        N_CORE_ASSERT(blueTexture->isReady,  "Blue texture should be ready");
        N_CORE_ASSERT(whiteTexture->isReady, "White texture should be ready");
        
        NL_INFO(GCoreLogger, "All texture uploads passed");
        NL_INFO(GCoreLogger, "  Red   bindless index: {}", redTexture->GetBindlessIndex());
        NL_INFO(GCoreLogger, "  Green bindless index: {}", greenTexture->GetBindlessIndex());
        NL_INFO(GCoreLogger, "  Blue  bindless index: {}", blueTexture->GetBindlessIndex());
        NL_INFO(GCoreLogger, "  White bindless index: {}", whiteTexture->GetBindlessIndex());
        
        myDevice->PrePresent();
        myDevice->EndFrame();
        
        myDevice->DestroyTexture(std::move(redTexture));
        myDevice->DestroyTexture(std::move(greenTexture));
        myDevice->DestroyTexture(std::move(blueTexture));
        myDevice->DestroyTexture(std::move(whiteTexture));
        
        // Constant buffer test — simple per-frame data
        struct TestConstants
        {
            float color[4];
            float time;
            float padding[3];
        };

        Nalta::RHI::BufferCreationDesc cbDesc{};
        cbDesc.size      = sizeof(TestConstants);
        cbDesc.access    = Nalta::RHI::BufferAccessFlags::CpuToGpu;
        cbDesc.viewFlags = Nalta::RHI::BufferViewFlags::ConstantBuffer;
        cbDesc.debugName = "TestConstantBuffer";

        auto constantBuffer = myDevice->CreateBuffer(cbDesc);
        N_CORE_ASSERT(constantBuffer != nullptr, "Failed to create constant buffer");
        N_CORE_ASSERT(constantBuffer->mappedData != nullptr, "Constant buffer should be mapped");
        N_CORE_ASSERT(constantBuffer->CBVDescriptor.IsValid(), "Constant buffer should have valid CBV");

        // Write data directly — no upload needed for CpuToGpu
        TestConstants data{};
        data.color[0] = 1.0f; // R
        data.color[1] = 0.5f; // G
        data.color[2] = 0.0f; // B
        data.color[3] = 1.0f; // A
        data.time     = 1.337f;

        memcpy(constantBuffer->mappedData, &data, sizeof(TestConstants));

        NL_INFO(GCoreLogger, "Constant buffer test passed");
        NL_INFO(GCoreLogger, "  CBV bindless index: {}", constantBuffer->CBVDescriptor.heapIndex);
        NL_INFO(GCoreLogger, "  GPU address:        {:#018x}", constantBuffer->gpuAddress);

        // Verify we can update it again — simulating per-frame update
        data.time = 2.674f;
        memcpy(constantBuffer->mappedData, &data, sizeof(TestConstants));
        NL_INFO(GCoreLogger, "  Constant buffer update passed");

        myDevice->DestroyBuffer(std::move(constantBuffer));
        
        NL_INFO(GCoreLogger, "initialized");
    }

    void GraphicsSystem::Shutdown()
    {
        NL_SCOPE_CORE("GraphicsSystem");
        
        myDevice->DestroyRenderSurface(std::move(surface));
        myDevice->DestroyContext(std::move(context));
        myDevice.reset();
        
        NL_INFO(GCoreLogger, "shutdown");
    }

    void GraphicsSystem::Test()
    {
        myDevice->BeginFrame();
        context->Reset();
        constexpr float clearColor[]{ 0.01f, 0.01f, 0.01f, 1.0f };
        
        surface->SetAsRenderTarget(*context);
        surface->Clear(*context, clearColor);
        
        surface->EndRenderTarget(*context);
        context->Close();
        myDevice->SubmitContextWork(*context);
        
        myDevice->PrePresent();
        surface->Present(true);
        myDevice->EndFrame();
    }
}
