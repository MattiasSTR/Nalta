#include "Nalta/Engine.h"

#include <iostream>
#include <thread>

namespace Nalta 
{
	void Engine::Run()
	{
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			myFrames[i].frameIndex = i;
		}
		
		std::thread gameThread(&Engine::GameLoop, this);
		std::thread renderThread(&Engine::RenderLoop, this);
		
		std::cin.get();
		myStop = true;
		
		gameThread.join();
		renderThread.join();
	}

	void Engine::GameLoop()
	{
		while (!myStop)
		{
			const int32_t frameIndex{ myCurrentFrame % static_cast<int32_t>(MAX_FRAMES_IN_FLIGHT) };
			FrameData& frame{ myFrames[frameIndex] };

			// Wait until this frame slot has been rendered (free)
			int32_t spinCount{ 0 };
			while (!frame.rendered.load(std::memory_order_acquire))
			{
				if (myStop)
				{
					return;
				}
				
				if (++spinCount < 1000) // short spin
				{
					std::this_thread::yield();
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::microseconds(10)); // adaptive sleep
				}
			}

			// Mark slot busy for game update
			frame.rendered = false;

			// Game update
			std::cout << "[Game] Updating frame " << frame.frameIndex << '\n';
			
			std::this_thread::sleep_for(std::chrono::microseconds(160)); 

			// Mark frame ready for render
			frame.ready.store(true, std::memory_order_release);

			++myCurrentFrame;
		}
	}

	void Engine::RenderLoop()
	{
		int32_t nextRenderFrame{ 0 };
		while (!myStop)
		{
			FrameData& frame{ myFrames[nextRenderFrame] };

			// Adaptive spin-wait until frame is ready
			int spinCount = 0;
			while (!frame.ready.load(std::memory_order_acquire))
			{
				if (myStop)
				{
					return;
				}

				if (++spinCount < 1000) // short spin
				{
					std::this_thread::yield();
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::microseconds(10)); // adaptive sleep
				}
			}

			// Rendering
			std::cout << "[Render] Rendering frame " << frame.frameIndex << '\n';

			// Mark slot free again
			frame.ready.store(false, std::memory_order_release);
			frame.rendered.store(true, std::memory_order_release);

			nextRenderFrame = (nextRenderFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		}
	}
}
