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

	const JsonConfig config = JsonConfig::LoadFromFile("config.json");
	const JsonConfig clientConfig = config.GetSection("client");
	const JsonConfig networkConfig = config.GetSection("network");

	NetworkSchedulerConfig netSchedulerConfig;
	netSchedulerConfig.runningThreadCount = 0;
	netSchedulerConfig.waitTimeoutMs = 16;
	netSchedulerConfig.tickIntervalMs = 16;

	ClientServiceConfig serviceConfig{};
	serviceConfig.netAddress = NetAddress(clientConfig.GetString("ip"), clientConfig.GetUInt16("port"));
	serviceConfig.maxSessionCount = clientConfig.GetInt32("maxSessionCount");
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netSchedulerConfig);
	serviceConfig.sessionFactory = []()->GameSessionRef
		{
			return cpp_net_engine::MakeShared<GameSession>(8192, 4096);
		};

	ClientServiceRef pService = cpp_net_engine::MakeShared<ClientService>(serviceConfig);

	if (pService->Start() == false)
	{
		NET_ENGINE_LOG_INFO("DummyClient start Failed");
		CrashReporter::Crash();
	}

	const int32 networkThreadCount = networkConfig.GetInt32("dispatchThreadCount");
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
