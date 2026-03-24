#include "pch.h"
#include "EchoServerService.h"
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
		ISingleton<EchoServerService>::GetInstance().Stop();
		return TRUE;
	default:
		return FALSE;
	}
}

int main()
{
	CrashReporter::Initialize("GameServer", "1.0.0", "");

	NetEngineInit netEngineInit;

	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

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
}
