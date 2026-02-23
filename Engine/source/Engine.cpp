#include "Nalta/Engine.h"

#include "Nalta/Core/Logger.h"

#include <thread>

namespace Nalta 
{
	Engine::Engine()
	{
		myLogger = std::make_unique<Logger>();
		myLogger->Init();
		GLogger = myLogger.get();
		
		const LoggerScope engineScope("Engine");
		NL_INFO("Engine constructed");
	}

	Engine::~Engine()
	{
		const LoggerScope engineScope("Engine");
		NL_INFO("Engine destroyed");
		
		GLogger = nullptr;
		myLogger->Shutdown();
		myLogger.reset();
	}

	void Engine::Run()
	{
		const LoggerScope engineScope("Engine::Run");
		NL_INFO("Starting run loop");
		
		NL_INFO("Creating Game & Render threads");
		std::thread gameThread(&Engine::GameLoop, this);
		std::thread renderThread(&Engine::RenderLoop, this);
		
		// Event Polling
		// File Watching
		// Asset Streaming
		while (!myStop)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}
		
		myStop = true;
		
		NL_INFO("Waiting for threads to finish");
		gameThread.join();
		renderThread.join();
		
		NL_INFO("Run loop finished");
	}

	void Engine::GameLoop()
	{
		const LoggerScope gameScope("GameLoop");
		NL_INFO("Game loop started");
		
		while (!myStop)
		{
			const int32_t frameIndex{ myCurrentFrame % static_cast<int32_t>(MAX_FRAMES_IN_FLIGHT) };
			FrameData& frame{ myFrames[frameIndex] };

			// Wait until this frame slot has been rendered (free)
			int32_t spinCount{ 0 };
			while (!frame.slotFree.load(std::memory_order_acquire))
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
			frame.slotFree = false;

			// UPDATE GAME

			// Mark frame ready for render
			frame.slotReady.store(true, std::memory_order_release);

			++myCurrentFrame;
		}
		
		NL_INFO("Game loop stopped");
	}

	void Engine::RenderLoop()
	{
		const LoggerScope renderScope("RenderLoop");
		NL_INFO("Render loop started");
		
		int32_t nextRenderFrame{ 0 };
		while (!myStop)
		{
			FrameData& frame{ myFrames[nextRenderFrame] };

			// Adaptive spin-wait until frame is ready
			int spinCount = 0;
			while (!frame.slotReady.load(std::memory_order_acquire))
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

			// RENDER GAME

			// Mark slot free again
			frame.slotReady.store(false, std::memory_order_release);
			frame.slotFree.store(true, std::memory_order_release);

			nextRenderFrame = (nextRenderFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		}
		
		NL_INFO("Render loop stopped");
	}
}
