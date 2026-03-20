#include "pch.h"

#include "GameSession.h"
#include "Service.h"
#include "ThreadManager.h"

#include "PacketHandler/PacketServiceTypeHandler.h"

int main()
{
	CrashReporter::Initialize("GameServer", "1.0.0", "");

	NetEngineInit netEngineInit;

	PacketServiceTypeHandler::Init();

	NetworkSchedulerConfig netConfig;
	netConfig.runningThreadCount = 0;
	netConfig.tickIntervalMs = 16;
	netConfig.waitTimeoutMs = 16;
	netConfig.onHandleError = [](const uint32 errorCode)->void
		{
		};

	ServerServiceConfig serviceConfig{};
	serviceConfig.netAddress = NetAddress("127.0.0.1", 7777);
	serviceConfig.acceptCount = 50;
	serviceConfig.maxSessionCount = 5000;
	serviceConfig.sessionTimeoutMs = 20000;
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netConfig);
	serviceConfig.sessionFactory = []()->GameSessionRef
		{
			return cpp_net_engine::MakeShared<GameSession>(8192, 4096);
		};

	const ServerServiceRef pService = cpp_net_engine::MakeShared<ServerService>(serviceConfig);

	if (pService->Start() == false)
	{
		NET_ENGINE_LOG_INFO("GameServer start is failed\n");
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

	ThreadManager::GetInstance().Launch("MonitorThread",[pService]()->void
		{
			while (true)
			{
				fmt::print("Current SessionCount : {}\n", pService->GetCurrentSessionCount());

				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
			}
		});

	ThreadManager::GetInstance().JoinWithClear();
}

