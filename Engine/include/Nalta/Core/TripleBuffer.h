#pragma once
#include <atomic>
#include <array>
#include <thread>

namespace Nalta
{
    // Non-blocking latest-frame-wins swap buffer.
    // The update thread always writes the freshest state.
    // The render thread always reads the latest available frame.
    // Neither thread ever blocks. Stale intermediate frames are discarded.
    
    template<typename T>
    class TripleBuffer
    {
    public:
        TripleBuffer()
        {
            myReadyIdx.store(2, std::memory_order_relaxed);
        }

        // Update thread only

        [[nodiscard]] T& GetWriteSlot()
        {
            return mySlots[myWriteIdx];
        }

        // Publish the written slot. Never blocks.
        void Publish()
        {
            myWriteIdx = myReadyIdx.exchange(myWriteIdx, std::memory_order_acq_rel);
        }

        // Render thread only

        // Returns true if a new frame was published since the last Consume().
        // If false, GetReadSlot() still returns the previous valid frame.
        bool Consume()
        {
            const size_t incoming{ myReadyIdx.exchange(myReadIdx, std::memory_order_acq_rel) };
            if (incoming == myReadIdx)
            {
                // Nothing new - restore the ready slot
                myReadyIdx.store(incoming, std::memory_order_release);
                return false;
            }
            myReadIdx = incoming;
            return true;
        }

        [[nodiscard]] const T& GetReadSlot() const
        {
            return mySlots[myReadIdx];
        }

    private:
        std::array<T, 3> mySlots{};
        std::atomic<size_t> myReadyIdx{ 2 };
        size_t myWriteIdx{ 0 }; // update thread only
        size_t myReadIdx { 1 }; // render thread only
    };
}