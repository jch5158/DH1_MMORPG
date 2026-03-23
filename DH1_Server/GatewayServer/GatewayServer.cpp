#include "pch.h"
#include "NetAddress.h"
#include "ThreadManager.h"
#include "NetworkScheduler.h"
#include "NetService.h"
#include "ClientSession.h"
#include "PacketServiceTypeHandler.h"
#include "ActorService.h"
#include "GatewayService.h"
#include "RedisActor.h"

class GatewayService;

int main()
{
	CrashReporter::Initialize("GatewayServer", "1.0.0", "");

	NetEngineInit netEngineInit;

	PacketServiceTypeHandler::Init();

	NetworkSchedulerConfig netConfig;
	netConfig.runningThreadCount = 0;
	netConfig.tickIntervalMs = 16;
	netConfig.waitTimeoutMs = 16;
	netConfig.onHandleError = [](const uint32)->void {};

	ServerServiceConfig serviceConfig;
	serviceConfig.acceptCount = 50;
	serviceConfig.maxSessionCount = 10000;
	serviceConfig.netAddress = NetAddress("127.0.0.1", 9000);
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netConfig);
	serviceConfig.sessionTimeoutMs = 60000;
	serviceConfig.sessionFactory = []()->ClientSessionRef
		{
			return cpp_net_engine::MakeShared<ClientSession>(8192, 4096);
		};

	ServerServiceRef pService = cpp_net_engine::MakeShared<ServerService>(eServiceType::Server);
	pService->Initialize(serviceConfig);

	ActorServiceConfig actorServiceConfig;
	actorServiceConfig.schedulerConfig.onHandleError = [](const uint32)->void {};
	actorServiceConfig.schedulerConfig.runningThreadCount = 0;
	actorServiceConfig.schedulerConfig.tickIntervalMs = 16;
	actorServiceConfig.schedulerConfig.waitTimeoutMs = 16;

	ActorServiceRef pActorService = cpp_net_engine::MakeShared<ActorService>();
	if (pActorService->Initialize(actorServiceConfig) == false)
	{
		CrashReporter::Crash();
	}
	pActorService->Start();

	RedisServiceRef pRedisService = cpp_net_engine::MakeShared<RedisService>("tcp://127.0.0.1:6379?pool_size=10", pActorService);

	if (ISingleton<GatewayService>::GetInstance().Initialize(pService, pRedisService) ==false)
	{
		CrashReporter::Crash();
	}

	if (pService->Start() == false)
	{
		NET_ENGINE_LOG_INFO("GameServer start is failed\n");
		CrashReporter::Crash();
	}

	for (int32 i = 0; i < 5; ++i)
	{
		ThreadManager::GetInstance().Launch("Network Dispatch",[pService]()->void
			{
				while (true)
				{
					pService->GetNetworkScheduler()->Dispatch();
				}
			});
	}

	for (int32 i = 0; i < 5; ++i)
	{
		ThreadManager::GetInstance().Launch("Actor Dispatch", [pActorService]()->void
			{
				while (true)
				{
					pActorService->Dispatch();
				}
			});
	}

	ThreadManager::GetInstance().Launch("Monitor",[pService]()->void
		{
			while (true)
			{
				fmt::print("Current SessionCount : {}\n", pService->GetCurrentSessionCount());

				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
			}
		});

	ThreadManager::GetInstance().JoinWithClear();
}
