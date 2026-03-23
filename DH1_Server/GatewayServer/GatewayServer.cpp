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
#include "JsonConfig.h"

class GatewayService;

int main()
{
	CrashReporter::Initialize("GatewayServer", "1.0.0", "");

	NetEngineInit netEngineInit;

	PacketServiceTypeHandler::Init();

	const JsonConfig config = JsonConfig::LoadFromFile("../../Shared/Config/Server/GatewayServerConfig.json");
	const JsonConfig serverConfig = config.GetSection("server");
	const JsonConfig sessionConfig = config.GetSection("session");
	const JsonConfig networkSchedulerConfig = config.GetSection("networkScheduler");
	const JsonConfig actorSchedulerConfig = config.GetSection("actorScheduler");
	const JsonConfig redisConfig = config.GetSection("redis");

	NetworkSchedulerConfig netConfig;
	netConfig.runningThreadCount = networkSchedulerConfig.GetUInt32("runningThreadCount");
	netConfig.waitTimeoutMs = networkSchedulerConfig.GetUInt32("waitTimeoutMs");
	netConfig.tickIntervalMs = networkSchedulerConfig.GetUInt32("tickIntervalMs");

	const int32 receiveBufferSize = sessionConfig.GetInt32("receiveBufferSize");
	const int32 sendBufferSize = sessionConfig.GetInt32("sendBufferSize");

	ServerServiceConfig serviceConfig;
	serviceConfig.acceptCount = serverConfig.GetInt32("acceptCount");
	serviceConfig.maxSessionCount = serverConfig.GetInt32("maxSessionCount");
	serviceConfig.netAddress = NetAddress(serverConfig.GetString("ip"), serverConfig.GetUInt16("port"));
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netConfig);
	serviceConfig.sessionTimeoutMs = serverConfig.GetInt64("sessionTimeoutMs");
	serviceConfig.sessionFactory = [receiveBufferSize, sendBufferSize]()->ClientSessionRef
		{
			return cpp_net_engine::MakeShared<ClientSession>(receiveBufferSize, sendBufferSize);
		};

	ServerServiceRef pService = cpp_net_engine::MakeShared<ServerService>(eServiceType::Server);
	pService->Initialize(serviceConfig);

	ActorServiceConfig actorSvcConfig;
	actorSvcConfig.schedulerConfig.runningThreadCount = actorSchedulerConfig.GetUInt32("runningThreadCount");
	actorSvcConfig.schedulerConfig.waitTimeoutMs = actorSchedulerConfig.GetUInt32("waitTimeoutMs");
	actorSvcConfig.schedulerConfig.tickIntervalMs = actorSchedulerConfig.GetUInt32("tickIntervalMs");

	ActorServiceRef pActorService = cpp_net_engine::MakeShared<ActorService>();
	if (pActorService->Initialize(actorSvcConfig) == false)
	{
		CrashReporter::Crash();
	}
	pActorService->Start();

	RedisServiceRef pRedisService = cpp_net_engine::MakeShared<RedisService>(redisConfig.GetString("connectionUri"), pActorService);

	if (ISingleton<GatewayService>::GetInstance().Initialize(pService, pRedisService) == false)
	{
		CrashReporter::Crash();
	}

	if (pService->Start() == false)
	{
		NET_ENGINE_LOG_INFO("GatewayServer start is failed\n");
		CrashReporter::Crash();
	}

	const int32 networkThreadCount = networkSchedulerConfig.GetInt32("dispatchThreadCount");
	for (int32 i = 0; i < networkThreadCount; ++i)
	{
		ThreadManager::GetInstance().Launch("Network Dispatch", [pService]()->void
			{
				while (true)
				{
					pService->GetNetworkScheduler()->Dispatch();
				}
			});
	}

	const int32 actorThreadCount = actorSchedulerConfig.GetInt32("dispatchThreadCount");
	for (int32 i = 0; i < actorThreadCount; ++i)
	{
		ThreadManager::GetInstance().Launch("Actor Dispatch", [pActorService]()->void
			{
				while (true)
				{
					pActorService->Dispatch();
				}
			});
	}

	ThreadManager::GetInstance().Launch("Monitor", [pService]()->void
		{
			while (true)
			{
				fmt::print("Current SessionCount : {}\n", pService->GetCurrentSessionCount());

				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
			}
		});

	ThreadManager::GetInstance().JoinWithClear();
}
