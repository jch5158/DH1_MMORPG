#include "pch.h"

#include "CrashReporter.h"
#include "GameSession.h"
#include "NetEngineInit.h"
#include "NetworkScheduler.h"
#include "NetService.h"
#include "ThreadManager.h"
#include "JsonConfig.h"

#include "PacketHandler/PacketServiceTypeHandler.h"

int32 main()
{
	CrashReporter::Initialize("DummyClient", "1.0.0", "");

	NetEngineInit netEngineInit;

	PacketServiceTypeHandler::Init();

	const JsonConfig config = JsonConfig::LoadFromFile("../../Shared/Config/Client/EchoClientConfig.json");
	const JsonConfig clientConfig = config.GetSection("client");
	const JsonConfig sessionConfig = config.GetSection("session");
	const JsonConfig networkSchedulerConfig = config.GetSection("networkScheduler");

	NetworkSchedulerConfig netConfig;
	netConfig.runningThreadCount = networkSchedulerConfig.GetUInt32("runningThreadCount");
	netConfig.waitTimeoutMs = networkSchedulerConfig.GetUInt32("waitTimeoutMs");
	netConfig.tickIntervalMs = networkSchedulerConfig.GetUInt32("tickIntervalMs");

	const int32 receiveBufferSize = sessionConfig.GetInt32("receiveBufferSize");
	const int32 sendBufferSize = sessionConfig.GetInt32("sendBufferSize");

	ClientServiceConfig serviceConfig{};
	serviceConfig.netAddress = NetAddress(clientConfig.GetString("ip"), clientConfig.GetUInt16("port"));
	serviceConfig.maxSessionCount = clientConfig.GetInt32("maxSessionCount");
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netConfig);
	serviceConfig.sessionFactory = [receiveBufferSize, sendBufferSize]()->GameSessionRef
		{
			return cpp_net_engine::MakeShared<GameSession>(receiveBufferSize, sendBufferSize);
		};

	ClientServiceRef pService = cpp_net_engine::MakeShared<ClientService>(eServiceType::Client);
	pService->Initialize(serviceConfig);

	if (pService->Start() == false)
	{
		NET_ENGINE_LOG_INFO("DummyClient start Failed");
		CrashReporter::Crash();
	}

	const int32 networkThreadCount = networkSchedulerConfig.GetInt32("dispatchThreadCount");
	for (int32 i = 0; i < networkThreadCount; ++i)
	{
		ThreadManager::GetInstance().Launch("NetWorkerThread", [pService]()->void
			{
				while (true)
				{
					pService->GetNetworkScheduler()->Dispatch();
				}
			});
	}

	ThreadManager::GetInstance().JoinWithClear();

	return 0;
}
