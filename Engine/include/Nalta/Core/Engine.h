#pragma once

#include "EngineConfig.h"
#include "FrameQueue.h"

#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>
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
	
	namespace Graphics
	{
		class RenderSurface;
	}
	
	class Logger;
	class IPlatformSystem;
	class GraphicsSystem;
	class IWindow;
	
	class Engine 
	{
	public:
		explicit Engine(const EngineConfig& aConfig);
		~Engine();
		
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
		std::vector<std::shared_ptr<IWindow>> myWindows;
		std::unique_ptr<GraphicsSystem> myGraphicsSystem;
		std::unordered_map<std::shared_ptr<IWindow>, std::shared_ptr<Graphics::RenderSurface>> myWindowSurfaces;
		
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
