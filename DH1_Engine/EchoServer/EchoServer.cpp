#include "pch.h"

#include "GameSession.h"
#include "NetEngineInit.h"
#include "NetworkScheduler.h"
#include "NetService.h"
#include "ThreadManager.h"
#include "JsonConfig.h"

#include "PacketHandler/PacketServiceTypeHandler.h"

int main()
{
	CrashReporter::Initialize("GameServer", "1.0.0", "");

	NetEngineInit netEngineInit;

	PacketServiceTypeHandler::Init();

	const JsonConfig config = JsonConfig::LoadFromFile("config.json");
	const JsonConfig serverConfig = config.GetSection("server");
	const JsonConfig networkConfig = config.GetSection("network");

	NetworkSchedulerConfig netSchedulerConfig;
	netSchedulerConfig.runningThreadCount = 0;
	netSchedulerConfig.tickIntervalMs = 16;
	netSchedulerConfig.waitTimeoutMs = 16;

	ServerServiceConfig serviceConfig{};
	serviceConfig.netAddress = NetAddress(serverConfig.GetString("ip"), serverConfig.GetUInt16("port"));
	serviceConfig.acceptCount = serverConfig.GetInt32("acceptCount");
	serviceConfig.maxSessionCount = serverConfig.GetInt32("maxSessionCount");
	serviceConfig.sessionTimeoutMs = serverConfig.GetInt64("sessionTimeoutMs");
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netSchedulerConfig);
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

	ThreadManager::GetInstance().Launch("MonitorThread", [pService]()->void
		{
			while (true)
			{
				fmt::print("Current SessionCount : {}\n", pService->GetCurrentSessionCount());

				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
			}
		});

	ThreadManager::GetInstance().JoinWithClear();
}
