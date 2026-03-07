#include <Nalta/Core/Engine.h>

int main(const int aArgc, char** aArgv)
{
	Nalta::EngineConfig config{ Nalta::GetDefaultGameConfig("Sandbox") };
	Nalta::ParseEngineConfig(config, aArgc, aArgv);
	
	bool restart{ true };
	while (restart)
	{
		Nalta::Engine engine{ config };
		engine.Initialize();
		engine.Run();
		engine.Shutdown();
		restart = engine.WantsRestart();
	}
	
	return 0;
}