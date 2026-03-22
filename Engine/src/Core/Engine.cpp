#include "npch.h"
#include "Nalta/Core/Engine.h"

#include "Nalta/Core/Timer.h"
#include "Nalta/Graphics/GraphicsSystem.h"
#include "Nalta/Graphics/IRenderContext.h"
#include "Nalta/Graphics/IRenderSurface.h"
#include "Nalta/Graphics/RenderSurfaceDesc.h"
#include "Nalta/Platform/IWindow.h"
#include "Nalta/Platform/PlatformSystemFactory.h"

#include <thread>

namespace Nalta 
{
	Engine::Engine(const EngineConfig& aConfig) : myConfig{ aConfig }
	{
		N_CORE_ASSERT(!aConfig.coreLogger.name.empty(), "Core Logger Must Have A Valid Name");
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
			myPlatformSystem = CreatePlatformSystem();
			myPlatformSystem->Initialize();
			
			const uint32_t coreCount{ myPlatformSystem->GetCPUCoreCount() };
			const uint64_t memoryBytes{ myPlatformSystem->GetSystemMemoryBytes() };
			NL_INFO(GCoreLogger, "CPU logical cores: {}", coreCount);
			NL_INFO(GCoreLogger, "System memory: {} MB", memoryBytes / (1024u * 1024u));
			
			myGraphicsSystem = std::make_unique<GraphicsSystem>();
			myGraphicsSystem->Initialize();
			
			myPlatformSystem->SetOnWindowDestroyedCallback([this](const WindowHandle aWindow)
			{
				if (myGraphicsSystem)
				{
					myGraphicsSystem->DestroySurface(aWindow);
				}
			});

			myMainWindow = myPlatformSystem->CreatePlatformWindow(*myConfig.mainWindowDesc);
			myMainWindow->Show();
			NL_INFO(GCoreLogger, "Main window created");

			Graphics::RenderSurfaceDesc surfaceDesc;
			surfaceDesc.window      = myMainWindow;
			surfaceDesc.bufferCount = 2;

			myMainSurface = myGraphicsSystem->CreateSurface(surfaceDesc);
			NL_INFO(GCoreLogger, "Main surface created");
		}
		else
		{
			NL_INFO(GCoreLogger, "Headless mode: skipping window system");
		}
	}

	void Engine::Shutdown()
	{
		const LoggerScope engineScope(GCoreLogger, "Engine::Shutdown");
		
		if (myGraphicsSystem)
		{
			myGraphicsSystem->Shutdown();
			myGraphicsSystem.reset();
			NL_INFO(GCoreLogger, "GraphicsSystem destroyed");
		}
		
		if (myPlatformSystem)
		{
			myPlatformSystem->SetOnWindowDestroyedCallback(nullptr);
			myPlatformSystem->Shutdown();
			myPlatformSystem.reset();
			NL_INFO(GCoreLogger, "PlatformSystem destroyed");
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
		if (myPlatformSystem != nullptr)
		{
			myPlatformSystem->SetCurrentThreadName("Main");
		}
		
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
		if (myPlatformSystem != nullptr)
		{
			myPlatformSystem->SetCurrentThreadName("Update");
		}
		
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
		if (myPlatformSystem != nullptr)
		{
			myPlatformSystem->SetCurrentThreadName("Render");
		}
		
		const LoggerScope renderScope(GCoreLogger, "RenderLoop");
		NL_INFO(GCoreLogger, "Render loop started");
		
		constexpr float clearColor[]{ 0.1f, 0.1f, 0.1f, 1.0f };
		
		RenderFrame frame;
		
		while (!myStop)
		{
			if (!myRenderQueue.Pop(frame))
			{
				break;
			}
			
			myGraphicsSystem->BeginFrame();
			myMainSurface->SetAsRenderTarget();
			myMainSurface->Clear(clearColor);
			myGraphicsSystem->GetRenderContext()->Execute(frame);
			myGraphicsSystem->EndFrame();
		}
		
		NL_INFO(GCoreLogger, "Render loop stopped");
	}
}
