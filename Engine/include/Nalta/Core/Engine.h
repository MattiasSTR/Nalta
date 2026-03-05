#pragma once

#include "FrameQueue.h"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace Nalta 
{
	// TEMP
	struct RenderFrame
	{
		struct MeshDraw
		{
			// mesh id
			// transform
			// material id
		};

		std::vector<MeshDraw> meshes;
	};
	
	
	class Logger;
	class IPlatformSystem;
	class IWindow;
	
	class Engine 
	{
	public:
		Engine();
		~Engine();
		
		void Initialize();
		void Shutdown();
		
		void Run();
		
		void RequestRestart() { myRestart = true; myStop = true; }
		bool WantsRestart() const { return myRestart.load(); }
		
	private:
		void UpdateLoop();
		void RenderLoop();
		
		std::unique_ptr<IPlatformSystem> myPlatformSystem;
		std::shared_ptr<IWindow> myMainWindow;
		std::vector<std::shared_ptr<IWindow>> myWindows;
		
		std::thread myUpdateThread;
		std::thread myRenderThread;
		
		std::atomic<bool> myStop{ false };
		std::atomic<bool> myRestart{ false };
		
		static constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2 };
		FrameQueue<RenderFrame> myRenderQueue{ MAX_FRAMES_IN_FLIGHT };
		
		std::unique_ptr<Logger> myCoreLogger;
		std::unique_ptr<Logger> myGameLogger;
	};
}
