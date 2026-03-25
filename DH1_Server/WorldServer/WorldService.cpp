#include "pch.h"
#include "WorldService.h"
#include "GatewaySession.h"
#include "NetService.h"
#include "NetworkScheduler.h"
#include "ActorService.h"
#include "ThreadManager.h"
#include "JsonConfig.h"

BOOL WINAPI WorldService::ConsoleCtrlHandler(const DWORD ctrlType)
{
	switch (ctrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
		GetInstance().Stop();
		return TRUE;
	default:
		return FALSE;
	}
}

bool WorldService::Initialize(const JsonConfig& config)
{
	const JsonConfig serverConfig = config.GetSection("server");
	const JsonConfig worldServerConfig = config.GetSection("worldServer");
	const JsonConfig networkSchedulerConfig = config.GetSection("networkScheduler");
	const JsonConfig actorSchedulerConfig = config.GetSection("actorScheduler");

	// NetworkScheduler
	NetworkSchedulerConfig netConfig;
	netConfig.runningThreadCount = networkSchedulerConfig.GetUInt32("runningThreadCount");
	netConfig.waitTimeoutMs = networkSchedulerConfig.GetUInt32("waitTimeoutMs");
	netConfig.tickIntervalMs = networkSchedulerConfig.GetUInt32("tickIntervalMs");

	// ClientService (connects TO GatewayServer)
	ClientServiceConfig serviceConfig;
	serviceConfig.netAddress = NetAddress(serverConfig.GetString("gatewayIp"), serverConfig.GetUInt16("gatewayPort"));
	serviceConfig.maxSessionCount = 1;
	serviceConfig.maxConnectionCount = 1;
	serviceConfig.bAutoReconnect = true;
	serviceConfig.reconnectIntervalMs = 3000;
	serviceConfig.maxReconnectCount = 0;
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netConfig);
	serviceConfig.sessionFactory = []() -> GatewaySessionRef
		{
			return cpp_net_engine::MakeShared<GatewaySession>(4096, 4096);
		};

	mpClientService = cpp_net_engine::MakeShared<ClientService>(eServiceType::Client);
	if (mpClientService->Initialize(serviceConfig) == false)
	{
		NET_ENGINE_LOG_ERROR("WorldService::Initialize - ClientService initialization failed");
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
		NET_ENGINE_LOG_ERROR("WorldService::Initialize - ActorService initialization failed");
		return false;
	}

	// Thread counts
	mNetworkDispatchThreadCount = networkSchedulerConfig.GetInt32("runningThreadCount");
	mActorDispatchThreadCount = actorSchedulerConfig.GetInt32("runningThreadCount");

	// WorldServer info
	mWorldServerId = worldServerConfig.GetInt32("worldServerId");

	NET_ENGINE_LOG_INFO("WorldService::Initialize - Initialization complete, worldServerId: {}", mWorldServerId);
	return true;
}

bool WorldService::Start()
{
	mpActorService->Start();

	if (mpClientService->Start() == false)
	{
		NET_ENGINE_LOG_ERROR("WorldService::Start - ClientService start failed");
		return false;
	}

	// Network dispatch threads
	for (int32 iter = 0; iter < mNetworkDispatchThreadCount; ++iter)
	{
		ThreadManager::GetInstance().Launch("Network Dispatch", [pScheduler = mpClientService->GetNetworkScheduler()]() -> void
			{
				while (!pScheduler->IsStopped())
				{
					pScheduler->Dispatch();
				}
			});
	}

	// Actor dispatch threads
	for (int32 iter = 0; iter < mActorDispatchThreadCount; ++iter)
	{
		ThreadManager::GetInstance().Launch("Actor Dispatch", [pActorService = mpActorService]() -> void
			{
				const auto pScheduler = pActorService->GetActorScheduler();
				while (!pScheduler->IsStopped())
				{
					pActorService->Dispatch();
				}
			});
	}

	mbRunning.store(true);

	NET_ENGINE_LOG_INFO("WorldService::Start - WorldServer started, worldServerId: {}", mWorldServerId);
	return true;
}

void WorldService::Run()
{
	while (mbRunning.load())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	NET_ENGINE_LOG_INFO("WorldService::Run - Closing service...");

	mpClientService->CloseService(mNetworkDispatchThreadCount);
	mpActorService->CloseService(mActorDispatchThreadCount);

	ThreadManager::GetInstance().JoinWithClear();

	NET_ENGINE_LOG_INFO("WorldService::Run - Shutdown complete");
}

void WorldService::Stop()
{
	mbRunning.store(false);
}

ClientServiceRef WorldService::GetClientServiceRef() const
{
	return mpClientService;
}

ActorServiceRef WorldService::GetActorServiceRef() const
{
	return mpActorService;
}
