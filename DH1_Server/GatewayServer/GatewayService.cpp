#include "pch.h"
#include "GatewayService.h"
#include "ClientSession.h"
#include "ClientSessionManager.h"
#include "NetService.h"
#include "NetworkScheduler.h"
#include "ActorService.h"
#include "ThreadManager.h"
#include "JsonConfig.h"

bool GatewayService::Initialize(const JsonConfig& config)
{
	const JsonConfig serverConfig = config.GetSection("server");
	const JsonConfig sessionConfig = config.GetSection("session");
	const JsonConfig networkSchedulerConfig = config.GetSection("networkScheduler");
	const JsonConfig actorSchedulerConfig = config.GetSection("actorScheduler");
	const JsonConfig redisConfig = config.GetSection("redis");

	// NetworkScheduler
	NetworkSchedulerConfig netConfig;
	netConfig.runningThreadCount = networkSchedulerConfig.GetUInt32("runningThreadCount");
	netConfig.waitTimeoutMs = networkSchedulerConfig.GetUInt32("waitTimeoutMs");
	netConfig.tickIntervalMs = networkSchedulerConfig.GetUInt32("tickIntervalMs");

	const int32 receiveBufferSize = sessionConfig.GetInt32("receiveBufferSize");
	const int32 sendBufferSize = sessionConfig.GetInt32("sendBufferSize");

	// ServerService
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
	serviceConfig.redisConnectionUri = redisConfig.GetString("connectionUri");

	mpServerService = cpp_net_engine::MakeShared<ServerService>(serviceConfig);
	if (mpServerService == nullptr)
	{
		NET_ENGINE_LOG_ERROR("GatewayService::Initialize - ServerService creation failed");
		return false;
	}

	// ActorService
	ActorServiceConfig actorSvcConfig;
	actorSvcConfig.schedulerConfig.runningThreadCount = actorSchedulerConfig.GetUInt32("runningThreadCount");
	actorSvcConfig.schedulerConfig.waitTimeoutMs = actorSchedulerConfig.GetUInt32("waitTimeoutMs");
	actorSvcConfig.schedulerConfig.tickIntervalMs = actorSchedulerConfig.GetUInt32("tickIntervalMs");

	mpActorService = cpp_net_engine::MakeShared<ActorService>();
	if (mpActorService->Initialize(actorSvcConfig) == false)
	{
		NET_ENGINE_LOG_ERROR("GatewayService::Initialize - ActorService initialization failed");
		return false;
	}

	// RedisService
	mpRedisService = cpp_net_engine::MakeShared<RedisService>(redisConfig.GetString("connectionUri"), mpActorService);

	// ClientSessionManager
	mpClientSessionManager = cpp_net_engine::MakeShared<ClientSessionManager>(serviceConfig.maxSessionCount);

	// Thread counts
	mNetworkDispatchThreadCount = networkSchedulerConfig.GetInt32("dispatchThreadCount");
	mActorDispatchThreadCount = actorSchedulerConfig.GetInt32("dispatchThreadCount");

	return true;
}

bool GatewayService::Start()
{
	mpActorService->Start();

	if (mpServerService->Start() == false)
	{
		NET_ENGINE_LOG_ERROR("GatewayService::Start - ServerService start failed");
		return false;
	}

	// Network dispatch threads
	for (int32 iter = 0; iter < mNetworkDispatchThreadCount; ++iter)
	{
		ThreadManager::GetInstance().Launch("Network Dispatch", [pService = mpServerService]()->void
			{
				while (true)
				{
					pService->GetNetworkScheduler()->Dispatch();
				}
			});
	}

	// Actor dispatch threads
	for (int32 iter = 0; iter < mActorDispatchThreadCount; ++iter)
	{
		ThreadManager::GetInstance().Launch("Actor Dispatch", [pActorService = mpActorService]()->void
			{
				while (true)
				{
					pActorService->Dispatch();
				}
			});
	}

	mbRunning.store(true);
	return true;
}

void GatewayService::Run()
{
	while (mbRunning.load())
	{
		fmt::print("Current SessionCount : {}\n", mpServerService->GetCurrentSessionCount());
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
	}

	ThreadManager::GetInstance().JoinWithClear();
}

void GatewayService::Stop()
{
	mbRunning.store(false);
}

ServerServiceRef GatewayService::GetServerServiceRef() const
{
	return mpServerService;
}

ActorServiceRef GatewayService::GetActorServiceRef() const
{
	return mpActorService;
}

ClientSessionManagerRef GatewayService::GetClientSessionManagerRef() const
{
	return mpClientSessionManager;
}

RedisServiceRef GatewayService::GetRedisServiceRef() const
{
	return mpRedisService;
}
