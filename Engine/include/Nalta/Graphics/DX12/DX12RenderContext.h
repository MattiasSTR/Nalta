#pragma once
#include "Nalta/Graphics/IRenderContext.h"

namespace Nalta::Graphics
{
    class DX12Device;

    class DX12RenderContext final : public IRenderContext
    {
    public:
        explicit DX12RenderContext(DX12Device* aDevice);
        ~DX12RenderContext() override;

        void Begin() override;
        void Execute(const RenderFrame& aFrame) override;

    private:
        DX12Device* myDevice{ nullptr };
    };
}