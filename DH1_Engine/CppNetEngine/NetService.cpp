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
	SessionRef pSession = mSessionFactory();
	if (pSession->Initialize(shared_from_this()) == false)
	{
		pSession = nullptr;
	}

	if (mpNetworkScheduler->Register(pSession) == false)
	{
		pSession = nullptr;
	}

	return pSession;
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

ClientService::ClientService(const eServiceType serviceType)
	: NetService(eServiceType::Client)
	, mpConnectionManager()
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
	pSession->Stop();

	mSessionManager.RemoveSession(pSession);

	mpConnectionManager->FreeConnection();
}

SessionRef ClientService::GetFirstSessionRef()
{
	return mSessionManager.GetFirstSessionRef();
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
	pSession->Stop();

	mSessionManager.RemoveSession(pSession);
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