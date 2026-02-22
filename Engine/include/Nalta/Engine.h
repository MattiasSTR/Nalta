#pragma once
#include <array>
#include <atomic>
#include <cstdint>

namespace Nalta 
{
	class Engine 
	{
	public:
		Engine();
		~Engine();
		
		void Run();
		
		void RequestRestart() { myRestart = true; myStop = true; }
		bool WantsRestart() const { return myRestart.load(); }
	private:
		void GameLoop();
		void RenderLoop();
		
		static constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 3 };

		struct FrameData
		{
			std::atomic<bool> ready{ false }; // game update done
			std::atomic<bool> rendered{ true };  // render done / slot free
		};

		std::array<FrameData, MAX_FRAMES_IN_FLIGHT> myFrames;
		std::atomic<int32_t> myCurrentFrame{ 0 };
		std::atomic<bool> myStop{ false };
		std::atomic<bool> myRestart{ false };
	};
}
