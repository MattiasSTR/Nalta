#pragma once
#include <atomic>
#include <cstdint>
#include <vector>

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
			uint32_t frameIndex{ 0 };
			std::atomic<bool> ready{ false }; // game update done
			std::atomic<bool> rendered{ true };  // render done / slot free
		};

		std::vector<FrameData> myFrames{ MAX_FRAMES_IN_FLIGHT };
		std::atomic<int32_t> myCurrentFrame{ 0 };
		std::atomic<bool> myStop{ false };
		std::atomic<bool> myRestart{ false };
	};
}
