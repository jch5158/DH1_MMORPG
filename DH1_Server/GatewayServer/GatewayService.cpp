#include "pch.h"
#include "GatewayService.h"
#include "ClientSession.h"
#include "WorldSession.h"
#include "ClientSessionManager.h"
#include "NetService.h"
#include "NetworkScheduler.h"
#include "ActorService.h"
#include "ThreadManager.h"
#include "JsonConfig.h"
#include "MySqlService.h"
#include "PacketHandler/ServerHeartbeatPacketHandler.h"

BOOL WINAPI GatewayService::ConsoleCtrlHandler(const DWORD ctrlType)
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

	// ServerService (2-phase init)
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

	mpServerService = cpp_net_engine::MakeShared<ServerService>(eServiceType::Server);
	if (mpServerService->Initialize(serviceConfig) == false)
	{
		NET_ENGINE_LOG_ERROR("GatewayService::Initialize - ServerService initialization failed");
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

	// WorldServer ClientService
	if (config.HasKey("worldServer"))
	{
		const JsonConfig worldServerConfig = config.GetSection("worldServer");

		NetworkSchedulerConfig worldNetConfig;
		worldNetConfig.runningThreadCount = worldServerConfig.GetUInt32("runningThreadCount");
		worldNetConfig.waitTimeoutMs = worldServerConfig.GetUInt32("waitTimeoutMs");
		worldNetConfig.tickIntervalMs = worldServerConfig.GetUInt32("tickIntervalMs");

		ClientServiceConfig worldServiceConfig;
		worldServiceConfig.netAddress = NetAddress(worldServerConfig.GetString("ip"), worldServerConfig.GetUInt16("port"));
		worldServiceConfig.maxSessionCount = 1;
		worldServiceConfig.maxConnectionCount = 1;
		worldServiceConfig.bAutoReconnect = true;
		worldServiceConfig.reconnectIntervalMs = worldServerConfig.GetInt64("reconnectIntervalMs");
		worldServiceConfig.maxReconnectCount = 0;
		worldServiceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(worldNetConfig);
		worldServiceConfig.sessionFactory = [receiveBufferSize, sendBufferSize]() -> WorldSessionRef
			{
				return cpp_net_engine::MakeShared<WorldSession>(receiveBufferSize, sendBufferSize);
			};

		mpWorldClientService = cpp_net_engine::MakeShared<ClientService>(eServiceType::Client);
		if (mpWorldClientService->Initialize(worldServiceConfig) == false)
		{
			NET_ENGINE_LOG_ERROR("GatewayService::Initialize - WorldServer ClientService initialization failed");
			return false;
		}
	}

	// RedisService
	mpRedisService = cpp_net_engine::MakeShared<RedisService>(redisConfig.GetString("connectionUri"), mpActorService);

	// MySqlService
	if (config.HasKey("mysql"))
	{
		const JsonConfig mysqlConfig = config.GetSection("mysql");
		MySqlConfig dbConfig;
		dbConfig.host = mysqlConfig.GetString("host");
		dbConfig.port = mysqlConfig.GetUInt16("port");
		dbConfig.user = mysqlConfig.GetString("user");
		dbConfig.password = mysqlConfig.GetString("password");
		dbConfig.database = mysqlConfig.GetString("database");

		mpMySqlService = cpp_net_engine::MakeShared<MySqlService>(dbConfig, mpActorService);
	}

	// AccountMySqlService (dh1_account_db)
	if (config.HasKey("accountMysql"))
	{
		const JsonConfig accountMysqlConfig = config.GetSection("accountMysql");
		MySqlConfig accountDbConfig;
		accountDbConfig.host = accountMysqlConfig.GetString("host");
		accountDbConfig.port = accountMysqlConfig.GetUInt16("port");
		accountDbConfig.user = accountMysqlConfig.GetString("user");
		accountDbConfig.password = accountMysqlConfig.GetString("password");
		accountDbConfig.database = accountMysqlConfig.GetString("database");

		mpAccountMySqlService = cpp_net_engine::MakeShared<MySqlService>(accountDbConfig, mpActorService);
	}

	// ClientSessionManager
	mpClientSessionManager = cpp_net_engine::MakeShared<ClientSessionManager>(serviceConfig.maxSessionCount);

	// Thread counts
	mNetworkDispatchThreadCount = networkSchedulerConfig.GetInt32("dispatchThreadCount");
	mActorDispatchThreadCount = actorSchedulerConfig.GetInt32("dispatchThreadCount");

	// Gateway info
	const JsonConfig gatewayConfig = config.GetSection("gateway");
	mGatewayId = gatewayConfig.GetInt32("gatewayId");
	mHeartbeatIntervalMs = gatewayConfig.GetInt64("heartbeatIntervalMs");
	mClientHeartbeatTimeoutMs = gatewayConfig.GetInt64("clientHeartbeatTimeoutMs");
	mWorldHeartbeatTimeoutMs = gatewayConfig.GetInt64("worldHeartbeatTimeoutMs");
	mRedisTtlSeconds = gatewayConfig.GetInt64("redisTtlSeconds");
	mSessionTtlSeconds = gatewayConfig.GetInt64("sessionTtlSeconds");
	mListenIp = serverConfig.GetString("ip");
	mListenPort = serverConfig.GetUInt16("port");
	mPublicIp = gatewayConfig.GetString("publicIp");

	NET_ENGINE_LOG_INFO("GatewayService::Initialize - Initialization complete, gatewayId: {}", mGatewayId);
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

	// WorldServer connection
	if (mpWorldClientService != nullptr)
	{
		if (mpWorldClientService->Start() == false)
		{
			NET_ENGINE_LOG_ERROR("GatewayService::Start - WorldServer ClientService start failed");
			return false;
		}

		ThreadManager::GetInstance().Launch("World Network Dispatch", [pScheduler = mpWorldClientService->GetNetworkScheduler()]() -> void
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
		ThreadManager::GetInstance().Launch("Network Dispatch", [pScheduler = mpServerService->GetNetworkScheduler()]()->void
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
		ThreadManager::GetInstance().Launch("Actor Dispatch", [pActorService = mpActorService]()->void
			{
				const auto pScheduler = pActorService->GetActorScheduler();
				while (!pScheduler->IsStopped())
				{
					pActorService->Dispatch();
				}
			});
	}

	mbRunning.store(true);

	NET_ENGINE_LOG_INFO("GatewayService::Start - Server started");
	return true;
}

void GatewayService::Run()
{
	updateGatewayRegistration(eGatewayStatus::Online);

	auto lastHeartbeat = std::chrono::steady_clock::now();

	while (mbRunning.load())
	{
		const auto now = std::chrono::steady_clock::now();
		const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeartbeat).count();

		if (elapsedMs >= mHeartbeatIntervalMs)
		{
			updateGatewayRegistration(eGatewayStatus::Online);
			sendHeartbeatToWorld();
			checkClientHeartbeats();
			checkWorldHeartbeats();
			refreshSessionTTLs();
			lastHeartbeat = now;

			NET_ENGINE_LOG_INFO("GatewayService::Run - SessionCount: {}, GatewayId: {}", mpServerService->GetCurrentSessionCount(), mGatewayId);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	updateGatewayRegistration(eGatewayStatus::ShuttingDown);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	NET_ENGINE_LOG_INFO("GatewayService - Closing service...");

	if (mpWorldClientService != nullptr)
	{
		mpWorldClientService->CloseService(1);
	}

	mpServerService->CloseService(mNetworkDispatchThreadCount);
	mpActorService->CloseService(mActorDispatchThreadCount);

	ThreadManager::GetInstance().JoinWithClear();

	NET_ENGINE_LOG_INFO("GatewayService - Shutdown complete");
}

void GatewayService::Stop()
{
	mbRunning.store(false);
}

ServerServiceRef GatewayService::GetServerServiceRef() const
{
	return mpServerService;
}

ClientServiceRef GatewayService::GetWorldClientServiceRef() const
{
	return mpWorldClientService;
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

MySqlServiceRef GatewayService::GetMySqlServiceRef() const
{
	return mpMySqlService;
}

MySqlServiceRef GatewayService::GetAccountMySqlServiceRef() const
{
	return mpAccountMySqlService;
}

int32 GatewayService::GetGatewayId() const
{
	return mGatewayId;
}

int32 GatewayService::GetWorldServerId() const
{
	if (mpWorldClientService == nullptr)
	{
		return 0;
	}

	const SessionRef pSession = mpWorldClientService->GetFirstSessionRef();
	if (pSession == nullptr)
	{
		return 0;
	}

	const auto pWorldSession = std::static_pointer_cast<WorldSession>(pSession);
	if (pWorldSession == nullptr)
	{
		return 0;
	}

	return pWorldSession->GetWorldServerId();
}

void GatewayService::checkClientHeartbeats()
{
	const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();

	const Vector<SessionRef> sessions = mpServerService->GetActiveSessions();

	for (const auto& pSession : sessions)
	{
		const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
		if (pClientSession == nullptr || !pClientSession->IsLoggedIn())
		{
			continue;
		}

		const int64 lastHeartbeatMs = pClientSession->GetLastHeartbeatMs();
		if (lastHeartbeatMs == 0)
		{
			continue;
		}

		const int64 elapsedMs = nowMs - lastHeartbeatMs;
		if (elapsedMs > mClientHeartbeatTimeoutMs)
		{
			NET_ENGINE_LOG_WARN("GatewayService::checkClientHeartbeats - Client heartbeat timeout, "
				"sessionId: {}, accountId: {}, elapsedMs: {}", pClientSession->GetSessionId(), pClientSession->GetAccountId(), elapsedMs);
			pClientSession->Disconnect(eDisconnectReason::Timeout);
		}
	}
}

void GatewayService::checkWorldHeartbeats()
{
	if (mpWorldClientService == nullptr)
	{
		return;
	}

	const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();

	const SessionRef pSession = mpWorldClientService->GetFirstSessionRef();
	if (pSession == nullptr)
	{
		return;
	}

	const auto pWorldSession = std::static_pointer_cast<WorldSession>(pSession);
	if (pWorldSession == nullptr)
	{
		return;
	}

	const int64 lastHeartbeatMs = pWorldSession->GetLastHeartbeatMs();
	if (lastHeartbeatMs == 0)
	{
		return;
	}

	const int64 elapsedMs = nowMs - lastHeartbeatMs;
	if (elapsedMs > mWorldHeartbeatTimeoutMs)
	{
		NET_ENGINE_LOG_WARN("GatewayService::checkWorldHeartbeats - WorldServer heartbeat timeout, elapsedMs: {}", elapsedMs);
		pWorldSession->Disconnect(eDisconnectReason::Timeout);
	}
}

void GatewayService::sendHeartbeatToWorld()
{
	if (mpWorldClientService == nullptr)
	{
		return;
	}

	const SessionRef pSession = mpWorldClientService->GetFirstSessionRef();
	if (pSession == nullptr)
	{
		return;
	}

	Protocol::S2S_HEARTBEAT_NOT packet;
	packet.set_servertype(static_cast<int32>(Protocol::eRole::GATEWAY_SERVER));
	packet.set_serverid(mGatewayId);
	packet.set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count());
	packet.set_sessioncount(mpServerService->GetCurrentSessionCount());
	packet.set_status(static_cast<int32>(eGatewayStatus::Online));

	pSession->Send(ServerHeartbeatPacketHandler::MakeSendBuffer(packet));
}

void GatewayService::updateGatewayRegistration(const eGatewayStatus status)
{
	if (mpRedisService == nullptr)
	{
		return;
	}

	RedisGatewayInfo info;
	info.gatewayId = mGatewayId;
	info.ip = mPublicIp;
	info.port = mListenPort;
	info.status = status;
	info.currentSessionCount = mpServerService->GetCurrentSessionCount();

	mpRedisService->UpdateGatewayInfo(info);
}

int64 GatewayService::GetSessionTtlSeconds() const
{
	return mSessionTtlSeconds;
}

void GatewayService::refreshSessionTTLs()
{
	if (mpRedisService == nullptr || mpClientSessionManager == nullptr)
	{
		return;
	}

	const Vector<uint64> accountIds = mpClientSessionManager->GetAllAccountIds();
	if (accountIds.empty())
	{
		return;
	}

	mpRedisService->RefreshSessionTTLAsync(accountIds, mSessionTtlSeconds);
}
