#include "npch.h"
#include "Nalta/Core/Engine.h"

#include "Nalta/Core/Window/WindowDesc.h"
#include "Nalta/Platform/WindowSystemFactory.h"

#include <iostream>
#include <thread>

namespace Nalta 
{
	Engine::Engine()
	{
		// Initialize Core Logger
		LoggerConfig coreConfig;
		coreConfig.pattern = "%^[%H:%M:%S:%e] [Thread %t] %n:%$ %v";
		coreConfig.fileName = "Nalta.log";
		coreConfig.name = "NALTA";

		myCoreLogger = std::make_unique<Logger>();
		myCoreLogger->Init(coreConfig);
		GCoreLogger = myCoreLogger.get();

		const LoggerScope engineScope(GCoreLogger, "Engine::Construct");
		NL_INFO(GCoreLogger, "Core Logger Created");

		// Initialize Game Logger
		LoggerConfig gameConfig;
		gameConfig.name = "GAME";
		gameConfig.pattern = "%^%n:%$ %v";

		myGameLogger = std::make_unique<Logger>();
		myGameLogger->Init(gameConfig);
		GGameLogger = myGameLogger.get();
		
		NL_INFO(GCoreLogger, "Game Logger Created");
	}

	Engine::~Engine() = default;
	
	void Engine::Initialize()
	{
		const LoggerScope engineScope(GCoreLogger, "Engine::Initialize");
		NL_INFO(GCoreLogger, "Initializing Engine Systems");
		
		myWindowSystem = CreateWindowSystem();
		myWindowSystem->Initialize();
		NL_INFO(GCoreLogger, "WindowSystem initialized");
		
		WindowDesc mainDesc;
		mainDesc.width = 1280;
		mainDesc.height = 720;
		mainDesc.caption = "Nalta";
		
		myWindows.push_back(myWindowSystem->CreateWindow(mainDesc));
		NL_INFO(GCoreLogger, "Main window created");
	}

	void Engine::Shutdown()
	{
		const LoggerScope engineScope(GCoreLogger, "Engine::Shutdown");
		
		myWindows.clear();
		
		if (myWindowSystem)
		{
			myWindowSystem->Shutdown();
			myWindowSystem.reset();
			NL_INFO(GCoreLogger, "WindowSystem destroyed");
		}
		
		if (myCoreLogger)
		{
			myCoreLogger->Shutdown();
			GCoreLogger = nullptr;
			myCoreLogger.reset();
		}

		if (myGameLogger)
		{
			myGameLogger->Shutdown();
			GGameLogger = nullptr;
			myGameLogger.reset();
		}
	}

	void Engine::Run()
	{
		const LoggerScope engineScope(GCoreLogger, "Engine::Run");
		NL_INFO(GCoreLogger, "Starting run loop");
		
		NL_INFO(GCoreLogger, "Creating Game & Render threads");
		std::thread gameThread(&Engine::GameLoop, this);
		std::thread renderThread(&Engine::RenderLoop, this);
		
		// Event Polling
		// File Watching
		// Asset Streaming
		
		// while (!myStop)
		// {
		// 	std::this_thread::sleep_for(std::chrono::microseconds(10));
		// }
		
		std::cin.get();
		
		myStop = true;
		
		NL_INFO(GCoreLogger, "Waiting for threads to finish");
		gameThread.join();
		renderThread.join();
		
		NL_INFO(GCoreLogger, "Run loop finished");
	}

	void Engine::GameLoop()
	{
		const LoggerScope gameScope(GCoreLogger, "GameLoop");
		NL_INFO(GCoreLogger, "Game loop started");
		
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
		
		NL_INFO(GCoreLogger, "Game loop stopped");
	}

	void Engine::RenderLoop()
	{
		const LoggerScope renderScope(GCoreLogger, "RenderLoop");
		NL_INFO(GCoreLogger, "Render loop started");
		
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
		
		NL_INFO(GCoreLogger, "Render loop stopped");
	}
}
