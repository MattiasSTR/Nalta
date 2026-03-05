#pragma once
#include <mutex>
#include <queue>

namespace Nalta
{
    template<typename T>
    class FrameQueue
    {
    public:
        explicit FrameQueue(const size_t aMaxFrames)
            : myMaxFrames(aMaxFrames)
        {}
        
        void Push(T aFrame)
        {
            std::unique_lock lock(myMutex);
            
            myFullCondition.wait(lock, [&]
            {
                return myQueue.size() < myMaxFrames || myStop;
            });

            if (myStop)
            {
                return;
            }

            myQueue.push(std::move(aFrame));
            myCondition.notify_one();
        }
        
        bool Pop(T& outFrame)
        {
            std::unique_lock lock(myMutex);
            
            myCondition.wait(lock, [&]
            {
                return !myQueue.empty() || myStop;
            });
            
            if (myQueue.empty())
            {
                return false;
            }
            
            outFrame = std::move(myQueue.front());
            myQueue.pop();
            myFullCondition.notify_one();
            return true;
        }
        
        void Stop()
        {
            std::unique_lock lock(myMutex);
            myStop = true;
            myCondition.notify_all();
            myFullCondition.notify_all();
        }
        
    private:
        std::queue<T> myQueue;
        std::mutex myMutex;
        std::condition_variable myCondition;     // Pop waits
        std::condition_variable myFullCondition; // Push waits
        bool myStop{ false };
        size_t myMaxFrames{ 2 };
    };
}
