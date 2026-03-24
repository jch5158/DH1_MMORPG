#include "pch.h"
#include "EchoClientService.h"
#include "JsonConfig.h"
#include "NetEngineInit.h"

#include "PacketHandler/PacketServiceTypeHandler.h"

BOOL WINAPI ConsoleCtrlHandler(const DWORD ctrlType)
{
	switch (ctrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
		ISingleton<EchoClientService>::GetInstance().Stop();
		return TRUE;
	default:
		return FALSE;
	}
}

int32 main()
{
	CrashReporter::Initialize("DummyClient", "1.0.0", "");

	NetEngineInit netEngineInit;

	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

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
}
