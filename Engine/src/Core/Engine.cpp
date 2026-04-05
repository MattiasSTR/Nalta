#include "npch.h"
#include "Nalta/Core/Engine.h"

#include "Nalta/Assets/AssetManager.h"
#include "Nalta/Core/InitContext.h"
#include "Nalta/Core/SceneViewContext.h"
#include "Nalta/Core/Timer.h"
#include "Nalta/Core/UpdateContext.h"
#include "Nalta/Graphics/GPUResourceManager.h"
#include "Nalta/Graphics/Renderer.h"
#include "Nalta/Input/InputSystem.h"
#include "Nalta/Platform/IWindow.h"
#include "Nalta/Platform/PlatformFactory.h"

#include <thread>

namespace Nalta 
{
	Engine::Engine(const EngineConfig& aConfig) : myConfig{ aConfig }
	{
		N_CORE_ASSERT(!aConfig.coreLogger.name.empty(), "Core Logger Must Have A Valid Name");
		myCoreLogger = std::make_unique<Logger>();
		myCoreLogger->Init(aConfig.coreLogger);
		GCoreLogger = myCoreLogger.get();
		
		NL_SCOPE_CORE("Engine::Construct");
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

	void Engine::Launch(const EngineConfig& aConfig)
	{
		bool restart{ true };
		while (restart)
		{
			Engine engine{ aConfig };
			engine.Initialize();
			engine.Run();
			engine.Shutdown();
			restart = engine.WantsRestart();
		}
	}

	void Engine::Initialize()
	{
		NL_SCOPE_CORE("Engine::Initialize");
		NL_INFO(GCoreLogger, "Initializing Engine Systems");
		
		if (myConfig.ShouldCreateWindow())
		{
			myPlatformSystem = CreatePlatformSystem();
			myPlatformSystem->Initialize();
			
			const uint32_t coreCount{ myPlatformSystem->GetCPUCoreCount() };
			const uint64_t memoryBytes{ myPlatformSystem->GetSystemMemoryBytes() };
			NL_INFO(GCoreLogger, "CPU logical cores: {}", coreCount);
			NL_INFO(GCoreLogger, "System memory: {} MB", memoryBytes / (1024u * 1024u));
			
			// myPlatformSystem->SetOnWindowDestroyedCallback([this](const WindowHandle aWindow)
			// {
			// 	if (myGraphicsSystem)
			// 	{
			// 		myGraphicsSystem->DestroySurface(aWindow);
			// 	}
			// });

			myMainWindow = myPlatformSystem->CreatePlatformWindow(*myConfig.mainWindowDesc);
			myMainWindow->Show();
			NL_INFO(GCoreLogger, "Main window created");
			
			myGPUResourceManager = std::make_unique<Graphics::GPUResourceManager>();
			myGPUResourceManager->Initialize();
			
			RHI::RenderSurfaceDesc desc;
			desc.width = myConfig.mainWindowDesc->width;
			desc.height = myConfig.mainWindowDesc->height;
			desc.window = myMainWindow->GetNativeHandle();
			myMainSurfaceKey = myGPUResourceManager->CreateRenderSurface(desc);
			
			auto& inputSystem{ myPlatformSystem->GetInputSystem() };
			myPlayerInput.AssignKeyboard(inputSystem.GetKeyboard());
			myPlayerInput.AssignMouse(inputSystem.GetMouse());
			
			myRenderer = std::make_unique<Graphics::Renderer>(myGPUResourceManager.get());
			myRenderer->Initialize();
		}
		else
		{
			NL_INFO(GCoreLogger, "Headless mode: skipping window system");
		}
		
		if (myConfig.gameFactory)
		{
			myGame = myConfig.gameFactory();

			InitContext initContext;
			initContext.assetManager = myAssetManager.get();
			myGame->Initialize(initContext);

			NL_INFO(GCoreLogger, "game initialized");
		}
	}

	void Engine::Shutdown()
	{
		NL_SCOPE_CORE("Engine::Shutdown");
		
		if (myGame)
		{
			myGame->Shutdown();
			myGame.reset();
			NL_INFO(GCoreLogger, "Game shutdown");
		}
		
		if (myRenderer)
		{
			myRenderer->Shutdown();
			myRenderer.reset();
		}
		
		if (myAssetManager)
		{
			myAssetManager->Shutdown();
			myAssetManager.reset();
			NL_INFO(GCoreLogger, "AssetManager shutdown");
		}
		
		if (myGPUResourceManager)
		{
			myGPUResourceManager->Shutdown();
			myGPUResourceManager.reset();
			NL_INFO(GCoreLogger, "GPUResourceSystem destroyed");
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
		NL_SCOPE_CORE("Engine::Run");
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
		
		NL_SCOPE_CORE("UpdateLoop");
		NL_INFO(GCoreLogger, "Update loop started");
		
		constexpr double fixedDelta{ 1.0 / 50.0 };
		Timer timer{ fixedDelta };
		
		UpdateContext updateContext;
		updateContext.playerInput = &myPlayerInput;
		
		const bool canRender{ myConfig.ShouldRunRenderThread() };
		while (!myStop)
		{
			myPlatformSystem->TickInput();
			timer.Update();
			
			while (timer.ShouldFixedUpdate())
			{
				// Fixed Update
				timer.ConsumeFixedUpdate();
			}
			
			if (myGame)
			{
				updateContext.deltaTime = static_cast<float>(timer.GetDeltaTime());
				myGame->Update(updateContext);
			}
			
			if (canRender)
			{
				Graphics::RenderFrame& frame{ myRenderBuffer.GetWriteSlot() };
				frame.Reset();
				
				if (myGame)
				{
					SceneViewContext ctx;
					ctx.view = &frame.scene;
					ctx.width = myMainWindow->GetWidth();
					ctx.height = myMainWindow->GetHeight();
					myGame->BuildSceneView(ctx);
				}
				
				// Assemble surface and camera from game data
				Graphics::SurfaceView& surfaceView{ frame.surfaces.emplace_back() };
				surfaceView.surface = myMainSurfaceKey;
				surfaceView.width = myMainWindow->GetWidth();
				surfaceView.height = myMainWindow->GetHeight();
				
				// Lift camera from SceneView into CameraView
				Graphics::CameraView& cameraView{ surfaceView.cameras.emplace_back() };
				cameraView.view = frame.scene.camera.view;
				cameraView.projection = frame.scene.camera.projection;
				cameraView.position = frame.scene.camera.position;

				myRenderBuffer.Publish();
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
		
		NL_SCOPE_CORE("RenderLoop");
		NL_INFO(GCoreLogger, "Render loop started");
		
		while (!myStop)
		{
			if (myRenderBuffer.Consume())
			{
				myRenderer->RenderFrame(myRenderBuffer.GetReadSlot());
			}
			else
			{
				std::this_thread::yield();
			}
		}
		
		NL_INFO(GCoreLogger, "Render loop stopped");
	}
}
