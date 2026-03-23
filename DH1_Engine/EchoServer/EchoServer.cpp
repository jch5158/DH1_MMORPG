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

	const JsonConfig config = JsonConfig::LoadFromFile("../../Shared/Config/Server/EchoServerConfig.json");
	const JsonConfig serverConfig = config.GetSection("server");
	const JsonConfig sessionConfig = config.GetSection("session");
	const JsonConfig networkSchedulerConfig = config.GetSection("networkScheduler");

	NetworkSchedulerConfig netConfig;
	netConfig.runningThreadCount = networkSchedulerConfig.GetUInt32("runningThreadCount");
	netConfig.waitTimeoutMs = networkSchedulerConfig.GetUInt32("waitTimeoutMs");
	netConfig.tickIntervalMs = networkSchedulerConfig.GetUInt32("tickIntervalMs");

	const int32 receiveBufferSize = sessionConfig.GetInt32("receiveBufferSize");
	const int32 sendBufferSize = sessionConfig.GetInt32("sendBufferSize");

	ServerServiceConfig serviceConfig{};
	serviceConfig.netAddress = NetAddress(serverConfig.GetString("ip"), serverConfig.GetUInt16("port"));
	serviceConfig.acceptCount = serverConfig.GetInt32("acceptCount");
	serviceConfig.maxSessionCount = serverConfig.GetInt32("maxSessionCount");
	serviceConfig.sessionTimeoutMs = serverConfig.GetInt64("sessionTimeoutMs");
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netConfig);
	serviceConfig.sessionFactory = [receiveBufferSize, sendBufferSize]()->GameSessionRef
		{
			return cpp_net_engine::MakeShared<GameSession>(receiveBufferSize, sendBufferSize);
		};

	const ServerServiceRef pService = cpp_net_engine::MakeShared<ServerService>(eServiceType::Server);
	pService->Initialize(serviceConfig);

	if (pService->Start() == false)
	{
		NET_ENGINE_LOG_INFO("GameServer start is failed\n");
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
