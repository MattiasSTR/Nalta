#pragma once
#include "Nalta/RHI/D3D12/Common/D3D12Common.h"

namespace Nalta::RHI::D3D12
{
    struct PipelineStateObject
    {
        ID3D12PipelineState* pipelineState{ nullptr };
        bool isCompute{ false };
        std::string debugName{};

        bool IsValid() const { return pipelineState != nullptr; }
    };
}