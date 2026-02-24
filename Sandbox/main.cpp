#include <Nalta/Core/Engine.h>

int main(int, char**)
{
	bool restart{ true };
	while (restart)
	{
		Nalta::Engine engine;
		engine.Run();

		restart = engine.WantsRestart();
	}
	
	return 0;
}