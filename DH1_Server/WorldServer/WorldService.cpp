#include "pch.h"
#include "WorldService.h"
#include "GatewaySession.h"
#include "RealmSession.h"
#include "RedisService.h"
#include "NetService.h"
#include "NetworkScheduler.h"
#include "ActorService.h"
#include "ThreadManager.h"
#include "JsonConfig.h"
#include "PacketHandler/ServerHeartbeatPacketHandler.h"
#include "Enum.pb.h"

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
	const JsonConfig sessionConfig = config.GetSection("session");
	const JsonConfig networkSchedulerConfig = config.GetSection("networkScheduler");
	const JsonConfig actorSchedulerConfig = config.GetSection("actorScheduler");

	// NetworkScheduler
	NetworkSchedulerConfig netConfig;
	netConfig.runningThreadCount = networkSchedulerConfig.GetUInt32("runningThreadCount");
	netConfig.waitTimeoutMs = networkSchedulerConfig.GetUInt32("waitTimeoutMs");
	netConfig.tickIntervalMs = networkSchedulerConfig.GetUInt32("tickIntervalMs");

	// ServerService (Listen for GatewayServer connections)
	mListenIp = serverConfig.GetString("ip");
	mListenPort = serverConfig.GetUInt16("port");

	const int32 receiveBufferSize = sessionConfig.GetInt32("receiveBufferSize");
	const int32 sendBufferSize = sessionConfig.GetInt32("sendBufferSize");

	ServerServiceConfig serviceConfig;
	serviceConfig.netAddress = NetAddress(mListenIp, mListenPort);
	serviceConfig.maxSessionCount = serverConfig.GetInt32("maxSessionCount");
	serviceConfig.acceptCount = serverConfig.GetInt32("acceptCount");
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netConfig);
	serviceConfig.sessionFactory = [receiveBufferSize, sendBufferSize]() -> GatewaySessionRef
		{
			return cpp_net_engine::MakeShared<GatewaySession>(receiveBufferSize, sendBufferSize);
		};

	mpServerService = cpp_net_engine::MakeShared<ServerService>(eServiceType::Server);
	if (mpServerService->Initialize(serviceConfig) == false)
	{
		NET_ENGINE_LOG_ERROR("WorldService::Initialize - ServerService initialization failed");
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

	// RedisService
	if (config.HasKey("redis"))
	{
		const JsonConfig redisConfig = config.GetSection("redis");
		mpRedisService = cpp_net_engine::MakeShared<RedisService>(redisConfig.GetString("connectionUri"), mpActorService);
	}

	// RealmServer ClientService
	if (config.HasKey("realmServer"))
	{
		const JsonConfig realmServerConfig = config.GetSection("realmServer");

		NetworkSchedulerConfig realmNetConfig;
		realmNetConfig.runningThreadCount = realmServerConfig.GetUInt32("runningThreadCount");
		realmNetConfig.waitTimeoutMs = realmServerConfig.GetUInt32("waitTimeoutMs");
		realmNetConfig.tickIntervalMs = realmServerConfig.GetUInt32("tickIntervalMs");

		ClientServiceConfig realmServiceConfig;
		realmServiceConfig.netAddress = NetAddress(realmServerConfig.GetString("ip"), realmServerConfig.GetUInt16("port"));
		realmServiceConfig.maxSessionCount = 1;
		realmServiceConfig.maxConnectionCount = 1;
		realmServiceConfig.bAutoReconnect = true;
		realmServiceConfig.reconnectIntervalMs = realmServerConfig.GetInt64("reconnectIntervalMs");
		realmServiceConfig.maxReconnectCount = 0;
		realmServiceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(realmNetConfig);
		realmServiceConfig.sessionFactory = [receiveBufferSize, sendBufferSize]() -> RealmSessionRef
			{
				return cpp_net_engine::MakeShared<RealmSession>(receiveBufferSize, sendBufferSize);
			};

		mpRealmClientService = cpp_net_engine::MakeShared<ClientService>(eServiceType::Client);
		if (mpRealmClientService->Initialize(realmServiceConfig) == false)
		{
			NET_ENGINE_LOG_ERROR("WorldService::Initialize - RealmServer ClientService initialization failed");
			return false;
		}
	}

	// Thread counts
	mNetworkDispatchThreadCount = networkSchedulerConfig.GetInt32("runningThreadCount");
	mActorDispatchThreadCount = actorSchedulerConfig.GetInt32("runningThreadCount");

	// WorldServer info
	mWorldServerId = worldServerConfig.GetInt32("worldServerId");
	mWorldName = worldServerConfig.GetString("worldName");
	mMaxPlayers = worldServerConfig.GetInt32("maxPlayers");
	mHeartbeatIntervalMs = worldServerConfig.GetInt64("heartbeatIntervalMs");
	mGatewayHeartbeatTimeoutMs = worldServerConfig.GetInt64("gatewayHeartbeatTimeoutMs");
	mRealmHeartbeatTimeoutMs = worldServerConfig.GetInt64("realmHeartbeatTimeoutMs");
	mRedisTtlSeconds = worldServerConfig.GetInt64("redisTtlSeconds");

	NET_ENGINE_LOG_INFO("WorldService::Initialize - Initialization complete, worldServerId: {}", mWorldServerId);
	return true;
}

bool WorldService::Start()
{
	mpActorService->Start();

	if (mpServerService->Start() == false)
	{
		NET_ENGINE_LOG_ERROR("WorldService::Start - ServerService start failed");
		return false;
	}

	// RealmServer connection
	if (mpRealmClientService != nullptr)
	{
		if (mpRealmClientService->Start() == false)
		{
			NET_ENGINE_LOG_ERROR("WorldService::Start - RealmServer ClientService start failed");
			return false;
		}

		ThreadManager::GetInstance().Launch("Realm Network Dispatch", [pScheduler = mpRealmClientService->GetNetworkScheduler()]() -> void
			{
				while (!pScheduler->IsStopped())
				{
					pScheduler->Dispatch();
				}
			});
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

	NET_ENGINE_LOG_INFO("WorldService::Start - Server started, worldServerId: {}, port: {}", mWorldServerId, mListenPort);
	return true;
}

void WorldService::Run()
{
	updateWorldRegistration(eWorldServerStatus::Online);

	auto lastHeartbeat = std::chrono::steady_clock::now();

	while (mbRunning.load())
	{
		const auto now = std::chrono::steady_clock::now();
		const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeartbeat).count();

		if (elapsedMs >= mHeartbeatIntervalMs)
		{
			updateWorldRegistration(eWorldServerStatus::Online);
			sendHeartbeatToRealm();
			sendHeartbeatToGateways();
			checkGatewayHeartbeats();
			checkRealmHeartbeat();
			lastHeartbeat = now;

			NET_ENGINE_LOG_INFO("WorldService::Run - SessionCount: {}, WorldServerId: {}", mpServerService->GetCurrentSessionCount(), mWorldServerId);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	updateWorldRegistration(eWorldServerStatus::ShuttingDown);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	NET_ENGINE_LOG_INFO("WorldService::Run - Closing service...");

	if (mpRealmClientService != nullptr)
	{
		mpRealmClientService->CloseService(1);
	}

	mpServerService->CloseService(mNetworkDispatchThreadCount);
	mpActorService->CloseService(mActorDispatchThreadCount);

	ThreadManager::GetInstance().JoinWithClear();

	NET_ENGINE_LOG_INFO("WorldService::Run - Shutdown complete");
}

void WorldService::Stop()
{
	mbRunning.store(false);
}

ServerServiceRef WorldService::GetServerServiceRef() const
{
	return mpServerService;
}

ClientServiceRef WorldService::GetRealmClientServiceRef() const
{
	return mpRealmClientService;
}

ActorServiceRef WorldService::GetActorServiceRef() const
{
	return mpActorService;
}

RedisServiceRef WorldService::GetRedisServiceRef() const
{
	return mpRedisService;
}

int32 WorldService::GetWorldServerId() const
{
	return mWorldServerId;
}

void WorldService::sendHeartbeatToRealm()
{
	if (mpRealmClientService == nullptr)
	{
		return;
	}

	const SessionRef pSession = mpRealmClientService->GetFirstSessionRef();
	if (pSession == nullptr)
	{
		return;
	}

	Protocol::S2S_HEARTBEAT_NOT packet;
	packet.set_servertype(static_cast<int32>(Protocol::eRole::WORLD_SERVER));
	packet.set_serverid(mWorldServerId);
	packet.set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count());
	packet.set_sessioncount(mpServerService->GetCurrentSessionCount());
	packet.set_status(static_cast<int32>(eWorldServerStatus::Online));

	pSession->Send(ServerHeartbeatPacketHandler::MakeSendBuffer(packet));
}

void WorldService::sendHeartbeatToGateways()
{
	const Vector<SessionRef> sessions = mpServerService->GetActiveSessions();

	Protocol::S2S_HEARTBEAT_NOT packet;
	packet.set_servertype(static_cast<int32>(Protocol::eRole::WORLD_SERVER));
	packet.set_serverid(mWorldServerId);
	packet.set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count());
	packet.set_sessioncount(mpServerService->GetCurrentSessionCount());
	packet.set_status(static_cast<int32>(eWorldServerStatus::Online));

	const auto sendBuffer = ServerHeartbeatPacketHandler::MakeSendBuffer(packet);

	for (const auto& pSession : sessions)
	{
		pSession->Send(sendBuffer);
	}
}

void WorldService::checkGatewayHeartbeats()
{
	const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();

	const Vector<SessionRef> sessions = mpServerService->GetActiveSessions();

	for (const auto& pSession : sessions)
	{
		const auto pGatewaySession = std::static_pointer_cast<GatewaySession>(pSession);
		if (pGatewaySession == nullptr)
		{
			continue;
		}

		const int64 lastHeartbeatMs = pGatewaySession->GetLastHeartbeatMs();
		if (lastHeartbeatMs == 0)
		{
			continue;
		}

		const int64 elapsedMs = nowMs - lastHeartbeatMs;
		if (elapsedMs > mGatewayHeartbeatTimeoutMs)
		{
			NET_ENGINE_LOG_WARN("WorldService::checkGatewayHeartbeats - GatewayServer heartbeat timeout, "
				"gatewayServerId: {}, elapsedMs: {}", pGatewaySession->GetGatewayServerId(), elapsedMs);
			pGatewaySession->Disconnect(eDisconnectReason::Timeout);
		}
	}
}

void WorldService::checkRealmHeartbeat()
{
	if (mpRealmClientService == nullptr)
	{
		return;
	}

	const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();

	const SessionRef pSession = mpRealmClientService->GetFirstSessionRef();
	if (pSession == nullptr)
	{
		return;
	}

	const auto pRealmSession = std::static_pointer_cast<RealmSession>(pSession);
	if (pRealmSession == nullptr)
	{
		return;
	}

	const int64 lastHeartbeatMs = pRealmSession->GetLastHeartbeatMs();
	if (lastHeartbeatMs == 0)
	{
		return;
	}

	const int64 elapsedMs = nowMs - lastHeartbeatMs;
	if (elapsedMs > mRealmHeartbeatTimeoutMs)
	{
		NET_ENGINE_LOG_WARN("WorldService::checkRealmHeartbeat - RealmServer heartbeat timeout, elapsedMs: {}", elapsedMs);
		pRealmSession->Disconnect(eDisconnectReason::Timeout);
	}
}

void WorldService::updateWorldRegistration(const eWorldServerStatus status)
{
	if (mpRedisService == nullptr)
	{
		return;
	}

	RedisWorldRegistration info;
	info.worldId = mWorldServerId;
	info.worldName = mWorldName;
	info.currentPlayers = mpServerService->GetCurrentSessionCount();
	info.maxPlayers = mMaxPlayers;
	info.status = static_cast<int32>(status);

	mpRedisService->UpdateWorldServerInfo(info, mRedisTtlSeconds);
}
