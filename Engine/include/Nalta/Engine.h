#pragma once
#include "Core/Containers/Vector.h"
#include "Core/Types/Atomic.h"
#include "Core/Types/Types.h"

namespace Nalta 
{
	class Engine 
	{
	public:
		void Run();
		
	private:
		void GameLoop();
		void RenderLoop();
		
		static constexpr UInt32 MAX_FRAMES_IN_FLIGHT{ 3 };

		struct FrameData
		{
			UInt32 frameIndex{ 0 };
			Atomic<bool> ready{ false }; // game update done
			Atomic<bool> rendered{ true };  // render done / slot free
		};

		Vector<FrameData> myFrames{ MAX_FRAMES_IN_FLIGHT };
		Atomic<Int32> myCurrentFrame{ 0 };
		Atomic<bool> myStop{ false };
	};
}