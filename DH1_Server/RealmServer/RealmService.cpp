#include "pch.h"
#include "RealmService.h"
#include "WorldServerSession.h"
#include "RealmSessionRegistry.h"
#include "NetService.h"
#include "NetworkScheduler.h"
#include "ActorService.h"
#include "ThreadManager.h"
#include "JsonConfig.h"
#include "PacketHandler/ServerHeartbeatPacketHandler.h"

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
	serviceConfig.sessionTimeoutMs = serverConfig.GetInt64("sessionTimeoutMs");
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
	mHeartbeatTimeoutMs = realmServerConfig.GetInt64("heartbeatTimeoutMs");
	mHeartbeatCheckIntervalMs = realmServerConfig.GetInt64("heartbeatCheckIntervalMs");
	mHeartbeatIntervalMs = realmServerConfig.GetInt64("heartbeatIntervalMs");

	// RealmSessionRegistry
	mpSessionRegistry = cpp_net_engine::MakeShared<RealmSessionRegistry>(1000);

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
	auto lastHeartbeatCheck = std::chrono::steady_clock::now();

	while (mbRunning.load())
	{
		const auto now = std::chrono::steady_clock::now();
		const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeartbeatCheck).count();

		if (elapsedMs >= mHeartbeatCheckIntervalMs)
		{
			sendHeartbeatToWorldServers();
			checkWorldServerHeartbeats();
			lastHeartbeatCheck = now;
		}

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

RealmSessionRegistryRef RealmService::GetSessionRegistryRef() const
{
	return mpSessionRegistry;
}

void RealmService::sendHeartbeatToWorldServers()
{
	const Vector<SessionRef> sessions = mpServerService->GetActiveSessions();

	Protocol::S2S_HEARTBEAT_NOT packet;
	packet.set_servertype(static_cast<int32>(Protocol::eRole::REALM_SERVER));
	packet.set_serverid(mRealmServerId);
	packet.set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count());
	packet.set_sessioncount(static_cast<int32>(sessions.size()));
	packet.set_status(static_cast<int32>(eRealmServerStatus::Online));

	const auto sendBuffer = ServerHeartbeatPacketHandler::MakeSendBuffer(packet);

	for (const auto& pSession : sessions)
	{
		pSession->Send(sendBuffer);
	}
}

void RealmService::checkWorldServerHeartbeats()
{
	const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();

	const Vector<SessionRef> sessions = mpServerService->GetActiveSessions();

	for (const auto& pSession : sessions)
	{
		const auto pWorldServerSession = std::static_pointer_cast<WorldServerSession>(pSession);
		if (pWorldServerSession == nullptr)
		{
			continue;
		}

		const int64 lastHeartbeatMs = pWorldServerSession->GetLastHeartbeatMs();
		if (lastHeartbeatMs == 0)
		{
			continue;
		}

		const int64 elapsedMs = nowMs - lastHeartbeatMs;
		if (elapsedMs > mHeartbeatTimeoutMs)
		{
			NET_ENGINE_LOG_WARN("RealmService::checkWorldServerHeartbeats - WorldServer heartbeat timeout, "
				"worldServerId: {}, elapsedMs: {}", pWorldServerSession->GetWorldServerId(), elapsedMs);
			pWorldServerSession->Disconnect(eDisconnectReason::Timeout);
		}
	}
}
