#pragma once

#include "EngineConfig.h"
#include "TripleBuffer.h"
#include "SceneView.h"
#include "Nalta/Core/IGame.h"
#include "Nalta/Platform/WindowHandle.h"
#include "Nalta/Input/PlayerInput.h"

#include <atomic>
#include <memory>
#include <thread>

namespace Nalta 
{
	namespace Graphics
	{
		class GPUResourceSystem;
	}
	
	class AssetManager;
	class Logger;
	class IPlatformSystem;
	class IWindow;
	
	class Engine 
	{
	public:
		explicit Engine(const EngineConfig& aConfig);
		~Engine();
		
		static void Launch(const EngineConfig& aConfig);
		
		void Initialize();
		void Shutdown();

		void Run();
		
		void RequestRestart() { myRestart = true; myStop = true; }
		bool WantsRestart() const { return myRestart.load(); }
		
	private:
		void RunClientLoop();
		void RunHeadlessLoop();
		void UpdateLoop();
		void RenderLoop();
		
		EngineConfig myConfig;
		
		std::unique_ptr<IPlatformSystem> myPlatformSystem;
		std::unique_ptr<Graphics::GPUResourceSystem> myGpuResourceSystem;
		std::unique_ptr<AssetManager> myAssetManager;
		
		WindowHandle myMainWindow;
		
		std::unique_ptr<IGame> myGame;
		
		std::thread myUpdateThread;
		std::thread myRenderThread;
		
		std::atomic<bool> myStop{ false };
		std::atomic<bool> myRestart{ false };
		
		PlayerInput myPlayerInput;

		TripleBuffer<SceneView> mySceneBuffer;
		
		std::unique_ptr<Logger> myCoreLogger;
		std::unique_ptr<Logger> myGameLogger;
	};
}
