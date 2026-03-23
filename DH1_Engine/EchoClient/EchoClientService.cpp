#include "pch.h"
#include "EchoClientService.h"
#include "GameSession.h"
#include "NetService.h"
#include "NetworkScheduler.h"
#include "ThreadManager.h"
#include "JsonConfig.h"

bool EchoClientService::Initialize(const JsonConfig& config)
{
	const JsonConfig clientConfig = config.GetSection("client");
	const JsonConfig sessionConfig = config.GetSection("session");
	const JsonConfig networkSchedulerConfig = config.GetSection("networkScheduler");

	// NetworkScheduler
	NetworkSchedulerConfig netConfig;
	netConfig.runningThreadCount = networkSchedulerConfig.GetUInt32("runningThreadCount");
	netConfig.waitTimeoutMs = networkSchedulerConfig.GetUInt32("waitTimeoutMs");
	netConfig.tickIntervalMs = networkSchedulerConfig.GetUInt32("tickIntervalMs");

	const int32 receiveBufferSize = sessionConfig.GetInt32("receiveBufferSize");
	const int32 sendBufferSize = sessionConfig.GetInt32("sendBufferSize");

	// ClientService
	ClientServiceConfig serviceConfig;
	serviceConfig.netAddress = NetAddress(clientConfig.GetString("ip"), clientConfig.GetUInt16("port"));
	serviceConfig.maxSessionCount = clientConfig.GetInt32("maxSessionCount");
	serviceConfig.maxConnectionCount = clientConfig.GetInt32("maxConnectionCount");
	serviceConfig.bAutoReconnect = clientConfig.GetBool("autoReconnect");
	serviceConfig.reconnectIntervalMs = clientConfig.GetInt64("reconnectIntervalMs");
	serviceConfig.maxReconnectCount = clientConfig.GetInt32("maxReconnectCount");
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netConfig);
	serviceConfig.sessionFactory = [receiveBufferSize, sendBufferSize]()->GameSessionRef
		{
			return cpp_net_engine::MakeShared<GameSession>(receiveBufferSize, sendBufferSize);
		};

	mpClientService = cpp_net_engine::MakeShared<ClientService>(eServiceType::Client);
	if (mpClientService->Initialize(serviceConfig) == false)
	{
		NET_ENGINE_LOG_ERROR("EchoClientService::Initialize - ClientService initialization failed");
		return false;
	}

	mNetworkDispatchThreadCount = networkSchedulerConfig.GetInt32("dispatchThreadCount");

	return true;
}

bool EchoClientService::Start()
{
	if (mpClientService->Start() == false)
	{
		NET_ENGINE_LOG_ERROR("EchoClientService::Start - ClientService start failed");
		return false;
	}

	for (int32 iter = 0; iter < mNetworkDispatchThreadCount; ++iter)
	{
		ThreadManager::GetInstance().Launch("Network Dispatch", [pService = mpClientService]()->void
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

void EchoClientService::Run()
{
	while (mbRunning.load())
	{
		fmt::print("Current SessionCount : {}\n", mpClientService->GetCurrentSessionCount());
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
	}

	ThreadManager::GetInstance().JoinWithClear();
}

void EchoClientService::Stop()
{
	mbRunning.store(false);
}

ClientServiceRef EchoClientService::GetClientServiceRef() const
{
	return mpClientService;
}
