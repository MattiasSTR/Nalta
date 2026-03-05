#include "npch.h"
#include "Nalta/Core/Engine.h"

#include "Nalta/Core/Timer.h"
#include "Nalta/Platform/IWindow.h"
#include "Nalta/Platform/PlatformSystemFactory.h"
#include "Nalta/Platform/WindowDesc.h"

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
		
		myPlatformSystem = CreateWindowSystem();
		myPlatformSystem->Initialize();
		NL_INFO(GCoreLogger, "WindowSystem initialized");
		
		WindowDesc mainDesc;
		mainDesc.width = 1280;
		mainDesc.height = 720;
		mainDesc.caption = "Nalta";
		
		myMainWindow = myPlatformSystem->CreateWindow(mainDesc);
		NL_INFO(GCoreLogger, "Main window created");
	}

	void Engine::Shutdown()
	{
		const LoggerScope engineScope(GCoreLogger, "Engine::Shutdown");
		
		myWindows.clear();
		myMainWindow.reset();
		NL_INFO(GCoreLogger, "Destroyed Windows");
		
		if (myPlatformSystem)
		{
			myPlatformSystem->Shutdown();
			myPlatformSystem.reset();
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
		myUpdateThread = std::thread(&Engine::UpdateLoop, this);
		myRenderThread = std::thread(&Engine::RenderLoop, this);
		
		// Event Polling
		// File Watching
		// Asset Streaming
		while (!myStop)
		{
			myPlatformSystem->PollEvents();

			std::erase_if(myWindows,[](const std::shared_ptr<IWindow>& aWindow)
			{
				return aWindow->IsClosed();
			});
			
			if (myMainWindow == nullptr || myMainWindow->IsClosed())
			{
				myStop = true;
				break;
			}
		}
		
		myRenderQueue.Stop();
		
		NL_INFO(GCoreLogger, "Waiting for threads to finish");
		myUpdateThread.join();
		myRenderThread.join();
		
		NL_INFO(GCoreLogger, "Run loop finished");
	}

	void Engine::UpdateLoop()
	{
		const LoggerScope gameScope(GCoreLogger, "GameLoop");
		NL_INFO(GCoreLogger, "Game loop started");
		
		constexpr double fixedDelta{ 1.0 / 50.0 };
		Timer timer{ fixedDelta };
		
		while (!myStop)
		{
			timer.Update();
			// Update
			
			while (timer.ShouldFixedUpdate())
			{
				// Fixed Update
				timer.ConsumeFixedUpdate();
			}
			
			RenderFrame frame;
			// Build Render Frame
			myRenderQueue.Push(std::move(frame));
		}
		
		NL_INFO(GCoreLogger, "Game loop stopped");
	}

	void Engine::RenderLoop()
	{
		const LoggerScope renderScope(GCoreLogger, "RenderLoop");
		NL_INFO(GCoreLogger, "Render loop started");
		
		RenderFrame frame;
		
		while (!myStop)
		{
			if (!myRenderQueue.Pop(frame))
			{
				break;
			}
			
			// Render frame
		}
		
		NL_INFO(GCoreLogger, "Render loop stopped");
	}
}
