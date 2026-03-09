#pragma once
#include "Buffer.h"

namespace Nalta::Graphics
{
    // Base Context class: command recording wrapper
    class Context
    {
    public:
        virtual ~Context() = default;

        virtual void Begin() = 0;
        virtual void End() = 0;
    };
    
    // GraphicsContext: for draw calls
    class GraphicsContext : public Context
    {
    public:
        ~GraphicsContext() override = default;

        virtual void SetVertexBuffer(Buffer* aBuffer) = 0;
        virtual void SetIndexBuffer(Buffer* aBuffer) = 0;
        virtual void Draw() = 0;

    private:
        Buffer* myVertexBuffer{ nullptr };
        Buffer* myIndexBuffer{ nullptr };
    };
    
    // ComputeContext: for compute/async workloads
    class ComputeContext : public Context
    {
    public:
        ~ComputeContext() override = default;

        virtual void Dispatch() = 0;
    };
    
    // UploadContext: for staging/upload operations
    class UploadContext : public Context
    {
    public:
        ~UploadContext() override = default;

        virtual void UploadBuffer(Buffer* aBuffer, void* aData) = 0;
    };
}