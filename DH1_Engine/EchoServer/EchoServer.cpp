#include "pch.h"
#include "EchoServerService.h"
#include "JsonConfig.h"
#include "NetEngineInit.h"

#include "PacketHandler/PacketServiceTypeHandler.h"

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

int main()
{
	CrashReporter::Initialize("GameServer", "1.0.0", "");

	NetEngineInit netEngineInit;

	SetConsoleCtrlHandler(EchoServerService::ConsoleCtrlHandler, TRUE);

	PacketServiceTypeHandler::Init();

	const JsonConfig config = JsonConfig::LoadFromFile(std::string(STRINGIFY(SOLUTION_DIR_UNQUOTED)) + "/Shared/Config/Server/EchoServerConfig.json");

	auto& service = ISingleton<EchoServerService>::GetInstance();

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
