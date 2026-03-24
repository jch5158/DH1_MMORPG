#include "pch.h"
#include "GatewayService.h"
#include "ClientSession.h"
#include "ClientSessionManager.h"
#include "NetService.h"
#include "NetworkScheduler.h"
#include "ActorService.h"
#include "ThreadManager.h"
#include "JsonConfig.h"
#include "MySqlService.h"

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

	// ClientSessionManager
	mpClientSessionManager = cpp_net_engine::MakeShared<ClientSessionManager>(serviceConfig.maxSessionCount);

	// Thread counts
	mNetworkDispatchThreadCount = networkSchedulerConfig.GetInt32("dispatchThreadCount");
	mActorDispatchThreadCount = actorSchedulerConfig.GetInt32("dispatchThreadCount");

	// Gateway info
	const JsonConfig gatewayConfig = config.GetSection("gateway");
	mGatewayId = gatewayConfig.GetInt32("gatewayId");
	mHeartbeatIntervalMs = gatewayConfig.GetInt64("heartbeatIntervalMs");
	mRedisTtlSeconds = gatewayConfig.GetInt64("redisTtlSeconds");
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
			lastHeartbeat = now;

			NET_ENGINE_LOG_INFO("GatewayService::Run - SessionCount: {}, GatewayId: {}", mpServerService->GetCurrentSessionCount(), mGatewayId);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	updateGatewayRegistration(eGatewayStatus::ShuttingDown);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	NET_ENGINE_LOG_INFO("GatewayService - Closing service...");

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
