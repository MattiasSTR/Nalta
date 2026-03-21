#pragma once

namespace Nalta::Graphics
{
    class IPipeline
    {
    public:
        virtual ~IPipeline() = default;

        [[nodiscard]] virtual bool IsValid() const = 0;
    };
}