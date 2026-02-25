#include <Nalta/Core/Engine.h>

int main(int, char**)
{
	bool restart{ true };
	while (restart)
	{
		Nalta::Engine engine;
		engine.Initialize();
		engine.Run();
		engine.Shutdown();
		restart = engine.WantsRestart();
	}
	
	return 0;
}