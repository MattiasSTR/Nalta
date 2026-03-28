#pragma once

#include "EngineConfig.h"
#include "FrameQueue.h"
#include "Nalta/Core/IGame.h"
#include "Nalta/Graphics/Commands/RenderFrame.h"
#include "Nalta/Graphics/RenderResources/DepthBufferHandle.h"
#include "Nalta/Graphics/Surface/RenderSurfaceHandle.h"
#include "Nalta/Platform/WindowHandle.h"
#include "Nalta/Input/PlayerInput.h"

#include <atomic>
#include <memory>
#include <thread>

namespace Nalta 
{
	namespace Graphics
	{
		class IRenderSurface;
	}
	
	class AssetManager;
	class Logger;
	class IPlatformSystem;
	class GraphicsSystem;
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
		std::unique_ptr<GraphicsSystem> myGraphicsSystem;
		std::unique_ptr<AssetManager> myAssetManager;
		
		WindowHandle myMainWindow;
		
		std::unique_ptr<IGame> myGame;
		
		std::thread myUpdateThread;
		std::thread myRenderThread;
		
		std::atomic<bool> myStop{ false };
		std::atomic<bool> myRestart{ false };
		Graphics::RenderSurfaceHandle myMainSurface;
		Graphics::DepthBufferHandle myMainDepthBuffer;
		
		PlayerInput myPlayerInput;

		static constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2 };
		FrameQueue<RenderFrame> myRenderQueue{ MAX_FRAMES_IN_FLIGHT };
		
		std::unique_ptr<Logger> myCoreLogger;
		std::unique_ptr<Logger> myGameLogger;
	};
}
