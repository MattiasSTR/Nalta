#include "SandboxGame.h"

#include <Nalta/Core/Engine.h>

int main(const int aArgc, char** aArgv)
{
	Nalta::EngineConfig config{ Nalta::GetDefaultGameConfig("Sandbox") };
	Nalta::ParseEngineConfig(config, aArgc, aArgv);
	config.gameFactory = []() { return std::make_unique<SandboxGame>(); };
	Nalta::Engine::Launch(config);
	return 0;
}