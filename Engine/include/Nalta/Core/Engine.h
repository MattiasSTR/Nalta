#pragma once

#include "EngineConfig.h"
#include "TripleBuffer.h"
#include "Nalta/Core/IGame.h"
#include "Nalta/Graphics/RenderFrame.h"
#include "Nalta/Platform/IWindow.h"
#include "Nalta/Input/PlayerInput.h"

#include <atomic>
#include <memory>
#include <thread>

namespace Nalta 
{
	namespace Graphics
	{
		class Renderer;
		class GPUResourceManager;
	}
	
	class IFileWatcher;
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
		std::unique_ptr<IFileWatcher> myFileWatcher;
		std::unique_ptr<Graphics::GPUResourceManager> myGPUResourceManager;
		std::unique_ptr<AssetManager> myAssetManager;
		std::unique_ptr<Graphics::Renderer> myRenderer;
		
		WindowKey myMainWindowKey;
		Graphics::RenderSurfaceKey myMainSurfaceKey;
		
		std::unique_ptr<IGame> myGame;
		
		std::thread myUpdateThread;
		std::thread myRenderThread;
		
		std::atomic<bool> myStop{ false };
		std::atomic<bool> myRestart{ false };
		
		PlayerInput myPlayerInput;

		TripleBuffer<Graphics::RenderFrame> myRenderBuffer;
		
		std::unique_ptr<Logger> myCoreLogger;
		std::unique_ptr<Logger> myGameLogger;
	};
}
