#include "pch.h"
#include "NetService.h"
#include "ConnectionPool.h"
#include "Listener.h"
#include "NetworkScheduler.h"
#include "Session.h"
#include "SessionReaper.h"

NetService::NetService(const eServiceType serviceType)
	: mbInitialize(false)
	  , mServiceType(serviceType)
	  , mNetAddress()
	  , mpNetworkScheduler()
	  , mSessionFactory()
	  , mSessionManager()
	  , mpRedisConnection()
	  , mpSessionIdAllocator()
	  , mpReusableSessionPool()
{
}

bool NetService::Initialize(const NetServiceConfig& config)
{
	if (mbInitialize.exchange(true) == true)
	{
		return false;
	}

	mNetAddress = config.netAddress;
	mpNetworkScheduler = config.pNetworkScheduler;
	mSessionFactory = config.sessionFactory;
	mSessionManager.SetMaxSessionCount(config.maxSessionCount);
	mpReusableSessionPool = cpp_net_engine::MakeUnique<LockFreeStack<SessionRef>>(config.maxSessionCount);

	mpSessionIdAllocator = cpp_net_engine::MakeShared<SessionIdAllocator>();

	if (!config.redisConnectionUri.empty())
	{
		mpRedisConnection = cpp_net_engine::MakeShared<RedisConnection>();
		if (!mpRedisConnection->Initialize(config.redisConnectionUri))
		{
			return false;
		}
	}

	if (!mpSessionIdAllocator->Initialize(mpRedisConnection.get()))
	{
		return false;
	}

	return true;
}

void NetService::Dispatch()
{
	if (mpNetworkScheduler == nullptr)
	{
		return;
	}

	mpNetworkScheduler->Dispatch();
}

SessionRef NetService::CreateSession()
{
	SessionRef pSession;
	if (mpReusableSessionPool->TryPop(pSession))
	{
		if (!pSession->Reset())
		{
			NET_ENGINE_LOG_ERROR("NetService::CreateSession - Session::Reset() failed, creating new session");
			return nullptr;
		}
	}
	else
	{
		pSession = mSessionFactory();
		if (pSession == nullptr || pSession->Initialize(shared_from_this()) == false)
		{
			return nullptr;
		}

		if (mpNetworkScheduler->Register(pSession) == false)
		{
			return nullptr;
		}
	}

	return std::move(pSession);  // NOLINT(clang-diagnostic-pessimizing-move)
}

bool NetService::IsInitialized() const
{
	return mbInitialize.load();
}

eServiceType NetService::GetServiceType() const
{
	return mServiceType;
}

NetAddress& NetService::GetNetAddress()
{
	return mNetAddress;
}

NetworkSchedulerRef NetService::GetNetworkScheduler() const
{
	return mpNetworkScheduler;
}

int32 NetService::GetCurrentSessionCount()
{
	return mSessionManager.GetCurrentSessionCount();
}

int32 NetService::GetMaxSessionCount() const
{
	return mSessionManager.GetMaxSessionCount();
}

uint64 NetService::AllocateSessionId() const
{
	if (mpSessionIdAllocator == nullptr)
	{
		return 0;
	}

	return mpSessionIdAllocator->Allocate();
}

void NetService::RecycleSession(const SessionRef& pSession) const
{
	pSession->Stop();
	(void)mpReusableSessionPool->TryPush(pSession);
}

ClientService::ClientService(const eServiceType serviceType)
	: NetService(eServiceType::Client)
	  , mpConnectionManager()
	  , mbAutoReconnect(false)
	  , mReconnectIntervalMs(0)
	  , mMaxReconnectCount(0)
	  , mReconnectAttemptCount(0)
{
}

bool ClientService::Initialize(const NetServiceConfig& config)
{
	if (!NetService::Initialize(config))
	{
		return false;
	}

	const auto* pClientConfig = static_cast<const ClientServiceConfig*>(&config);
	mpConnectionManager = cpp_net_engine::MakeShared<ConnectionPool>(pClientConfig->maxConnectionCount);
	mbAutoReconnect = pClientConfig->bAutoReconnect;
	mReconnectIntervalMs = pClientConfig->reconnectIntervalMs;
	mMaxReconnectCount = pClientConfig->maxReconnectCount;
	return true;
}

bool ClientService::Start()
{
	if (!IsInitialized())
	{
		return false;
	}

	const int32 maxConnectionCount = mpConnectionManager->GetMaxConnectionCount();
	for (int32 i = 0; i < maxConnectionCount; ++i)
	{
		mpConnectionManager->Connect(std::static_pointer_cast<ClientService>(shared_from_this()));
	}

	return true;
}

void ClientService::CloseService()
{
}

void ClientService::OnSessionConnected(const SessionRef& pSession)
{
	if (mSessionManager.AddSession(pSession) == false)
	{
		pSession->Disconnect(eDisconnectReason::ServerFull);
		return;
	}

	pSession->Start();
}

void ClientService::OnSessionDisconnected(const SessionRef& pSession)
{
	mSessionManager.RemoveSession(pSession);

	RecycleSession(pSession);

	mpConnectionManager->FreeConnection();

	if (mbAutoReconnect)
	{
		scheduleReconnect();
	}
}

SessionRef ClientService::GetFirstSessionRef()
{
	return mSessionManager.GetFirstSessionRef();
}

void ClientService::scheduleReconnect()
{
	if (mMaxReconnectCount > 0)
	{
		const int32 attemptCount = mReconnectAttemptCount.fetch_add(1) + 1;
		if (attemptCount > mMaxReconnectCount)
		{
			NET_ENGINE_LOG_ERROR("ClientService::scheduleReconnect - Max reconnect count reached ({})", mMaxReconnectCount);
			return;
		}
	}

	ClientServiceWeak pWeak = std::static_pointer_cast<ClientService>(shared_from_this());
	mpNetworkScheduler->RegisterDelay([pWeak]()->void
	{
		const ClientServiceRef pService = pWeak.lock();
		if (pService == nullptr)
		{
			return;
		}

		pService->mpConnectionManager->Connect(pService);
	}, mReconnectIntervalMs);
}

ServerService::ServerService(const eServiceType serviceType)
	:NetService(serviceType)
	 , mpListener()
	 , mpSessionReaper()
{
}

bool ServerService::Initialize(const NetServiceConfig& config)
{
	if (!NetService::Initialize(config))
	{
		return false;
	}

	const auto* pServerConfig = static_cast<const ServerServiceConfig*>(&config);
	mpListener = cpp_net_engine::MakeShared<Listener>(pServerConfig->acceptCount);
	mpSessionReaper = cpp_net_engine::MakeShared<SessionReaper>(pServerConfig->sessionTimeoutMs);
	return true;
}

bool ServerService::Start()
{
	if (!IsInitialized())
	{
		return false;
	}

	if (mpListener == nullptr)
	{
		return false;
	}

	const ServerServiceRef pServerService = std::static_pointer_cast<ServerService>(shared_from_this());
	if (mpListener->StartAccept(pServerService) == false)
	{
		return false;
	}

	registerReaperSweep();

	return true;
}

void ServerService::CloseService()
{
}

void ServerService::OnSessionConnected(const SessionRef& pSession)
{
	if (mSessionManager.AddSession(pSession) == false)
	{
		pSession->Disconnect(eDisconnectReason::ServerFull);
		return;
	}

	pSession->Start();
}

void ServerService::OnSessionDisconnected(const SessionRef& pSession)
{
	mSessionManager.RemoveSession(pSession);

	RecycleSession(pSession);
}

Vector<SessionRef> ServerService::GetActiveSessions()
{
	return mSessionManager.GetActiveSessions();
}

void ServerService::registerReaperSweep()
{
	ServerServiceWeak pWeak = std::static_pointer_cast<ServerService>(shared_from_this());
	mpNetworkScheduler->RegisterDelay([pWeak]()->void
	{
		const ServerServiceRef pService = pWeak.lock();
		if (pService == nullptr)
		{
			return;
		}

		pService->mpSessionReaper->Sweep(pWeak);
		pService->mpSessionReaper->SweepAbort();

		pService->registerReaperSweep();
	}, SessionReaper::GetSweepIntervalMs());
}
