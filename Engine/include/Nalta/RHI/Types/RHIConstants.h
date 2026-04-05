#pragma once
#include <cstdint>

namespace Nalta::RHI
{
    constexpr uint32_t FRAMES_IN_FLIGHT{ 2 };
    constexpr uint32_t BACK_BUFFER_COUNT{ 3 };
    constexpr uint32_t ROOT_CONSTANT_COUNT{ 16 };
    constexpr uint32_t MAX_QUEUED_BARRIERS{ 16 };

    enum class RootParameter : uint32_t
    {
        Constants = 0,
        PassCBV = 1,
    };
    
    struct ContextSubmissionResult
    {
        uint32_t frameId{ 0 };
        uint32_t submissionIndex{ 0 };
    };
    
    enum class ContextWaitType : uint8_t
    {
        Host = 0,
        Graphics,
        Compute,
        Copy
    };
}