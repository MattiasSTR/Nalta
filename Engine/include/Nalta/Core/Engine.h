#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

namespace Nalta 
{
	class Logger;
	class IWindowSystem;
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
		void GameLoop();
		void RenderLoop();
		
		std::unique_ptr<Logger> myCoreLogger;
		std::unique_ptr<Logger> myGameLogger;
		
		std::unique_ptr<IWindowSystem> myWindowSystem;
		std::vector<std::shared_ptr<IWindow>> myWindows;
		
		static constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 3 };

		struct FrameData
		{
			std::atomic<bool> slotReady{ false }; // game update done
			std::atomic<bool> slotFree{ true };  // render done / slot free
		};

		std::array<FrameData, MAX_FRAMES_IN_FLIGHT> myFrames;
		std::atomic<int32_t> myCurrentFrame{ 0 };
		std::atomic<bool> myStop{ false };
		std::atomic<bool> myRestart{ false };
	};
}
