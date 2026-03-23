#include "pch.h"
#include "EchoServerService.h"
#include "JsonConfig.h"
#include "PacketHandler/PacketServiceTypeHandler.h"

int main()
{
	CrashReporter::Initialize("GameServer", "1.0.0", "");

	NetEngineInit netEngineInit;

	PacketServiceTypeHandler::Init();

	const JsonConfig config = JsonConfig::LoadFromFile("../../Shared/Config/Server/EchoServerConfig.json");

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
	service.Stop();
}
