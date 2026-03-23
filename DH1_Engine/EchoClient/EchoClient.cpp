#include "pch.h"
#include "EchoClientService.h"
#include "JsonConfig.h"
#include "NetEngineInit.h"

#include "PacketHandler/PacketServiceTypeHandler.h"

int32 main()
{
	CrashReporter::Initialize("DummyClient", "1.0.0", "");

	NetEngineInit netEngineInit;

	PacketServiceTypeHandler::Init();

	const JsonConfig config = JsonConfig::LoadFromFile("../../Shared/Config/Client/EchoClientConfig.json");

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
	service.Stop();
}
