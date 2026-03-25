#include "pch.h"
#include "RealmService.h"
#include "WorldServerSession.h"
#include "NetService.h"
#include "NetworkScheduler.h"
#include "ActorService.h"
#include "ThreadManager.h"
#include "JsonConfig.h"

BOOL WINAPI RealmService::ConsoleCtrlHandler(const DWORD ctrlType)
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

bool RealmService::Initialize(const JsonConfig& config)
{
	const JsonConfig serverConfig = config.GetSection("server");
	const JsonConfig realmServerConfig = config.GetSection("realmServer");
	const JsonConfig sessionConfig = config.GetSection("session");
	const JsonConfig networkSchedulerConfig = config.GetSection("networkScheduler");
	const JsonConfig actorSchedulerConfig = config.GetSection("actorScheduler");

	// NetworkScheduler
	NetworkSchedulerConfig netConfig;
	netConfig.runningThreadCount = networkSchedulerConfig.GetUInt32("runningThreadCount");
	netConfig.waitTimeoutMs = networkSchedulerConfig.GetUInt32("waitTimeoutMs");
	netConfig.tickIntervalMs = networkSchedulerConfig.GetUInt32("tickIntervalMs");

	// ServerService (Listen for WorldServer connections)
	mListenIp = serverConfig.GetString("ip");
	mListenPort = serverConfig.GetUInt16("port");

	const int32 receiveBufferSize = sessionConfig.GetInt32("receiveBufferSize");
	const int32 sendBufferSize = sessionConfig.GetInt32("sendBufferSize");

	ServerServiceConfig serviceConfig;
	serviceConfig.netAddress = NetAddress(mListenIp, mListenPort);
	serviceConfig.maxSessionCount = serverConfig.GetInt32("maxSessionCount");
	serviceConfig.acceptCount = serverConfig.GetInt32("acceptCount");
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netConfig);
	serviceConfig.sessionFactory = [receiveBufferSize, sendBufferSize]() -> WorldServerSessionRef
		{
			return cpp_net_engine::MakeShared<WorldServerSession>(receiveBufferSize, sendBufferSize);
		};

	mpServerService = cpp_net_engine::MakeShared<ServerService>(eServiceType::Server);
	if (mpServerService->Initialize(serviceConfig) == false)
	{
		NET_ENGINE_LOG_ERROR("RealmService::Initialize - ServerService initialization failed");
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
		NET_ENGINE_LOG_ERROR("RealmService::Initialize - ActorService initialization failed");
		return false;
	}

	// Thread counts
	mNetworkDispatchThreadCount = networkSchedulerConfig.GetInt32("runningThreadCount");
	mActorDispatchThreadCount = actorSchedulerConfig.GetInt32("runningThreadCount");

	// RealmServer info
	mRealmServerId = realmServerConfig.GetInt32("realmServerId");

	NET_ENGINE_LOG_INFO("RealmService::Initialize - Initialization complete, realmServerId: {}", mRealmServerId);
	return true;
}

bool RealmService::Start()
{
	mpActorService->Start();

	if (mpServerService->Start() == false)
	{
		NET_ENGINE_LOG_ERROR("RealmService::Start - ServerService start failed");
		return false;
	}

	// Network dispatch threads
	for (int32 iter = 0; iter < mNetworkDispatchThreadCount; ++iter)
	{
		ThreadManager::GetInstance().Launch("Network Dispatch", [pScheduler = mpServerService->GetNetworkScheduler()]() -> void
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

	NET_ENGINE_LOG_INFO("RealmService::Start - Server started, realmServerId: {}, port: {}", mRealmServerId, mListenPort);
	return true;
}

void RealmService::Run()
{
	while (mbRunning.load())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	NET_ENGINE_LOG_INFO("RealmService::Run - Closing service...");

	mpServerService->CloseService(mNetworkDispatchThreadCount);
	mpActorService->CloseService(mActorDispatchThreadCount);

	ThreadManager::GetInstance().JoinWithClear();

	NET_ENGINE_LOG_INFO("RealmService::Run - Shutdown complete");
}

void RealmService::Stop()
{
	mbRunning.store(false);
}

ServerServiceRef RealmService::GetServerServiceRef() const
{
	return mpServerService;
}

ActorServiceRef RealmService::GetActorServiceRef() const
{
	return mpActorService;
}
