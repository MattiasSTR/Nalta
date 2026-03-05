#pragma once

namespace Nalta
{
    class Timer
    {
    public:
        using Clock = std::chrono::high_resolution_clock;

        explicit Timer(const double aFixedDeltaSeconds = 1.0 / 50.0) : myFixedDelta(aFixedDeltaSeconds)
        {
            Reset();
        }
        
        void Reset()
        {
            myLastTime = Clock::now();
            myTotalTime = 0.0;
            myDeltaTime = 0.0;
            myAccumulator = 0.0;
            myFixedUpdatesPending = 0;
        }

        // Call once per frame
        void Update()
        {
            const auto now{ Clock::now() };
            const std::chrono::duration<double> dt{ now - myLastTime };
            myLastTime = now;

            myDeltaTime = dt.count();
            myTotalTime += myDeltaTime;
            myAccumulator += myDeltaTime;
            
            myFixedUpdatesPending = static_cast<int>(myAccumulator / myFixedDelta);
            myAccumulator -= myFixedUpdatesPending * myFixedDelta;
        }

        double GetDeltaTime() const { return myDeltaTime; }
        double GetTotalTime() const { return myTotalTime; }
        double GetFixedDelta() const { return myFixedDelta; }
        double GetAlpha() const { return myAccumulator / myFixedDelta; }

        bool ShouldFixedUpdate() const { return myFixedUpdatesPending > 0; }
        void ConsumeFixedUpdate() { if (myFixedUpdatesPending > 0) --myFixedUpdatesPending; }
        
    private:
        std::chrono::time_point<Clock> myLastTime;
        double myTotalTime{ 0.0 };
        double myDeltaTime{ 0.0 };
        double myAccumulator{ 0.0 };
        double myFixedDelta{ 1.0 / 50.0 };
        int myFixedUpdatesPending{ 0 };
    };
}