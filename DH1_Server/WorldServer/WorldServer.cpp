#include "pch.h"
#include "WorldService.h"
#include "JsonConfig.h"
#include "NetEngineInit.h"

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

int main()
{
	CrashReporter::Initialize("WorldServer", "1.0.0", "");

	NetEngineInit netEngineInit;

	SetConsoleCtrlHandler(WorldService::ConsoleCtrlHandler, TRUE);

	const JsonConfig config = JsonConfig::LoadFromFile(std::string(STRINGIFY(SOLUTION_DIR_UNQUOTED)) + "/Shared/Config/Server/WorldServerConfig.json");

	auto& service = ISingleton<WorldService>::GetInstance();

	if (service.Initialize(config) == false)
	{
		CrashReporter::Crash();
	}

	if (service.Start() == false)
	{
		CrashReporter::Crash();
	}

	service.Run();
}
