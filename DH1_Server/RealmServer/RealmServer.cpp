#include "pch.h"
#include "RealmService.h"
#include "PacketHandler/PacketServiceTypeHandler.h"
#include "JsonConfig.h"
#include "NetEngineInit.h"

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

int main()
{
	CrashReporter::Initialize("RealmServer", "1.0.0", "");

	NetEngineInit netEngineInit;

	SetConsoleCtrlHandler(RealmService::ConsoleCtrlHandler, TRUE);

	const JsonConfig config = JsonConfig::LoadFromFile(std::string(STRINGIFY(SOLUTION_DIR_UNQUOTED)) + "/Shared/Config/Server/RealmServerConfig.json");

	PacketServiceTypeHandler::Init();

	auto& service = ISingleton<RealmService>::GetInstance();

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
