#pragma once
#include <memory>

struct ID3D12Device10;
struct ID3D12CommandQueue;
struct ID3D12GraphicsCommandList7;

namespace Nalta::Graphics
{
    class DX12CopyQueue
    {
    public:
        DX12CopyQueue();
        ~DX12CopyQueue();

        void Initialize(ID3D12Device10* aDevice) const;
        void Shutdown() const;

        // Submit recorded commands and signal the fence
        void Execute() const;

        // Wait on CPU until all submitted work is complete
        void WaitForCompletion() const;

        // Signal and wait — convenience for synchronous uploads
        void ExecuteAndWait() const;

        // Open the command list for recording
        void Begin() const;

        // Close the command list
        void End() const;

        [[nodiscard]] ID3D12CommandQueue* GetQueue() const;
        [[nodiscard]] ID3D12GraphicsCommandList7* GetCommandList() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> myImpl;
    };
}