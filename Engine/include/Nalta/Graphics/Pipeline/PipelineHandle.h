#pragma once

namespace Nalta::Graphics
{
    class IPipeline;

    class PipelineHandle
    {
    public:
        PipelineHandle() = default;
        explicit PipelineHandle(IPipeline* aPipeline) : myPipeline(aPipeline) {}

        [[nodiscard]] bool IsValid() const { return myPipeline != nullptr; }
        [[nodiscard]] IPipeline* Get() const { return myPipeline; }

        IPipeline* operator->() const { return myPipeline; }
        explicit operator bool() const { return IsValid(); }

        bool operator==(const PipelineHandle& aOther) const { return myPipeline == aOther.myPipeline; }
        bool operator!=(const PipelineHandle& aOther) const { return !(*this == aOther); }

    private:
        IPipeline* myPipeline{ nullptr };
    };
}