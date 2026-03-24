#include "pch.h"
#include "GatewayService.h"
#include "JsonConfig.h"
#include "PacketServiceTypeHandler.h"

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

int main()
{
	CrashReporter::Initialize("GatewayServer", "1.0.0", "");

	NetEngineInit netEngineInit;

	PacketServiceTypeHandler::Init();

	const JsonConfig config = JsonConfig::LoadFromFile(std::string(STRINGIFY(SOLUTION_DIR_UNQUOTED)) + "/Shared/Config/Server/GatewayServerConfig.json");

	auto& service = ISingleton<GatewayService>::GetInstance();

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
