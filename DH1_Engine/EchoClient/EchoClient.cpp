#include "pch.h"

#include "CrashReporter.h"
#include "GameSession.h"
#include "NetEngineInit.h"
#include "NetworkScheduler.h"
#include "Service.h"
#include "ThreadManager.h"

#include "PacketHandler/PacketServiceTypeHandler.h"

int32 main()
{
	CrashReporter::Initialize("DummyClient", "1.0.0", "");

	NetEngineInit netEngineInit;

	PacketServiceTypeHandler::Init();

	NetworkSchedulerConfig netConfig;
	netConfig.runningThreadCount = 0;
	netConfig.waitTimeoutMs = 16;
	netConfig.tickIntervalMs = 16;
	netConfig.onHandleError = [](const uint32 errorCode)->void
		{
		};

	ClientServiceConfig serviceConfig{};
	serviceConfig.netAddress = NetAddress("127.0.0.1", 7777);
	serviceConfig.maxSessionCount = 5000;
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netConfig);
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

	for (int32 i = 0; i < 5; ++i)
	{
		ThreadManager::GetInstance().Launch("NetWorkerThread",[pService]()->void
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
