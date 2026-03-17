#include "npch.h"
#include "Nalta/Core/Engine.h"

#include "Nalta/Core/Timer.h"
#include "Nalta/Graphics/GraphicsSystem.h"
#include "Nalta/Platform/IWindow.h"
#include "Nalta/Platform/PlatformSystemFactory.h"

#include <thread>

namespace Nalta 
{
	Engine::Engine(const EngineConfig& aConfig) : myConfig{ aConfig }
	{
		N_ASSERT(!aConfig.coreLogger.name.empty(), "Core Logger Must Have A Valid Name");
		myCoreLogger = std::make_unique<Logger>();
		myCoreLogger->Init(aConfig.coreLogger);
		GCoreLogger = myCoreLogger.get();
		
		const LoggerScope engineScope(GCoreLogger, "Engine::Construct");
		NL_INFO(GCoreLogger, "Core Logger Created");

		if (!aConfig.gameLogger.name.empty())
		{
			myGameLogger = std::make_unique<Logger>();
			myGameLogger->Init(aConfig.gameLogger);
			GGameLogger = myGameLogger.get();
			NL_INFO(GCoreLogger, "Game Logger Created");
		}
		
		NL_INFO(GCoreLogger, "Engine Created");
	}

	Engine::~Engine() = default;
	
	void Engine::Initialize()
	{
		const LoggerScope engineScope(GCoreLogger, "Engine::Initialize");
		NL_INFO(GCoreLogger, "Initializing Engine Systems");
		
		if (myConfig.ShouldCreateWindow())
		{
			myPlatformSystem = CreateWindowSystem();
			myPlatformSystem->Initialize();
			NL_INFO(GCoreLogger, "WindowSystem initialized");

			myMainWindow = myPlatformSystem->CreatePlatformWindow(*myConfig.mainWindowDesc);
			myMainWindow->Show();
			NL_INFO(GCoreLogger, "Main window created");
			
			//myGraphicsSystem = std::make_unique<GraphicsSystem>();
			//myGraphicsSystem->Initialize();
			NL_INFO(GCoreLogger, "GraphicsSystem initialized");

			//const auto surface{ myGraphicsSystem->CreateRenderSurface(window) };
			//myWindowSurfaces[window] = surface;
			NL_INFO(GCoreLogger, "RenderSurface created for main window");
		}
		else
		{
			NL_INFO(GCoreLogger, "Headless mode: skipping window system");
		}
	}

	void Engine::Shutdown()
	{
		const LoggerScope engineScope(GCoreLogger, "Engine::Shutdown");
		
		NL_INFO(GCoreLogger, "Destroyed Windows & Surfaces");
		
		if (myPlatformSystem)
		{
			myPlatformSystem->Shutdown();
			myPlatformSystem.reset();
			NL_INFO(GCoreLogger, "WindowSystem destroyed");
		}
		
		// if (myGraphicsSystem)
		// {
		// 	myGraphicsSystem->Shutdown();
		// 	myGraphicsSystem.reset();
		// 	NL_INFO(GCoreLogger, "GraphicsSystem destroyed");
		// }
		
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

		if (myConfig.mode == EngineMode::Client)
		{
			RunClientLoop();
		}
		else
		{
			RunHeadlessLoop();
		}

		NL_INFO(GCoreLogger, "Run loop finished");
	}

	void Engine::RunClientLoop()
	{
		NL_INFO(GCoreLogger, "Running client loop with rendering");
		myUpdateThread = std::thread(&Engine::UpdateLoop, this);
		myRenderThread = std::thread(&Engine::RenderLoop, this);
		
		while (!myStop)
		{
			if (!myPlatformSystem->PollEvents())
			{
				myStop = true;
			}
		}
		
		myRenderQueue.Stop();

		if (myUpdateThread.joinable())
		{
			myUpdateThread.join();
		}
		if (myRenderThread.joinable())
		{
			myRenderThread.join();
		}
	}

	void Engine::RunHeadlessLoop()
	{
		NL_INFO(GCoreLogger, "Running headless loop (server)");
		
		UpdateLoop();
	}

	void Engine::UpdateLoop()
	{
		const LoggerScope gameScope(GCoreLogger, "UpdateLoop");
		NL_INFO(GCoreLogger, "Update loop started");
		
		constexpr double fixedDelta{ 1.0 / 50.0 };
		Timer timer{ fixedDelta };
		
		const bool canRender{ myConfig.ShouldRunRenderThread() };
		while (!myStop)
		{
			timer.Update();
			
			while (timer.ShouldFixedUpdate())
			{
				// Fixed Update
				timer.ConsumeFixedUpdate();
			}
			
			if (canRender)
			{
				RenderFrame frame;
				// Build RenderFrame
				myRenderQueue.Push(std::move(frame));
			}
		}
		
		NL_INFO(GCoreLogger, "Update loop stopped");
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
			
			// for (auto& surface : myWindowSurfaces | std::views::values)
			// {
			// 	myGraphicsSystem->Present(surface);
			// }
		}
		
		NL_INFO(GCoreLogger, "Render loop stopped");
	}
}
