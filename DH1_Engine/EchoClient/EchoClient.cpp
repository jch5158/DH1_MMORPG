#include "pch.h"
#include "EchoClientService.h"
#include "JsonConfig.h"
#include "NetEngineInit.h"

#include "PacketHandler/PacketServiceTypeHandler.h"

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

int32 main()
{
	CrashReporter::Initialize("DummyClient", "1.0.0", "");

	NetEngineInit netEngineInit;

	SetConsoleCtrlHandler(EchoClientService::ConsoleCtrlHandler, TRUE);

	PacketServiceTypeHandler::Init();

	const JsonConfig config = JsonConfig::LoadFromFile(std::string(STRINGIFY(SOLUTION_DIR_UNQUOTED)) + "/Shared/Config/Client/EchoClientConfig.json");

	auto& service = ISingleton<EchoClientService>::GetInstance();

	if (service.Initialize(config) == false)
	{
		CrashReporter::Crash();
	}

	if (service.Start() == false)
	{
		NET_ENGINE_LOG_INFO("DummyClient start Failed");
		CrashReporter::Crash();
	}

	service.Run();
}
