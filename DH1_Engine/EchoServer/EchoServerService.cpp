#include "pch.h"
#include "EchoServerService.h"
#include "GameSession.h"
#include "NetService.h"
#include "NetworkScheduler.h"
#include "ThreadManager.h"
#include "JsonConfig.h"

bool EchoServerService::Initialize(const JsonConfig& config)
{
	const JsonConfig serverConfig = config.GetSection("server");
	const JsonConfig sessionConfig = config.GetSection("session");
	const JsonConfig networkSchedulerConfig = config.GetSection("networkScheduler");
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
	serviceConfig.netAddress = NetAddress(serverConfig.GetString("ip"), serverConfig.GetUInt16("port"));
	serviceConfig.acceptCount = serverConfig.GetInt32("acceptCount");
	serviceConfig.maxSessionCount = serverConfig.GetInt32("maxSessionCount");
	serviceConfig.sessionTimeoutMs = serverConfig.GetInt64("sessionTimeoutMs");
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netConfig);
	serviceConfig.sessionFactory = [receiveBufferSize, sendBufferSize]()->GameSessionRef
		{
			return cpp_net_engine::MakeShared<GameSession>(receiveBufferSize, sendBufferSize);
		};
	serviceConfig.redisConnectionUri = redisConfig.GetString("connectionUri");

	mpServerService = cpp_net_engine::MakeShared<ServerService>(eServiceType::Server);
	if (mpServerService->Initialize(serviceConfig) == false)
	{
		NET_ENGINE_LOG_ERROR("EchoServerService::Initialize - ServerService initialization failed");
		return false;
	}

	mNetworkDispatchThreadCount = networkSchedulerConfig.GetInt32("dispatchThreadCount");

	return true;
}

bool EchoServerService::Start()
{
	if (mpServerService->Start() == false)
	{
		NET_ENGINE_LOG_ERROR("EchoServerService::Start - ServerService start failed");
		return false;
	}

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

	mbRunning.store(true);
	return true;
}

void EchoServerService::Run()
{
	while (mbRunning.load())
	{
		fmt::print("Current SessionCount : {}\n", mpServerService->GetCurrentSessionCount());
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
	}

	ThreadManager::GetInstance().JoinWithClear();
}

void EchoServerService::Stop()
{
	mbRunning.store(false);
}

ServerServiceRef EchoServerService::GetServerServiceRef() const
{
	return mpServerService;
}
