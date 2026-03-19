#include "pch.h"
#include "Service.h"
#include "Listener.h"
#include "Session.h"
#include "SessionReaper.h"
#include <utility>

Service::Service(
	const eServiceType serviceType,
	const NetAddress& netAddress,
	const int32 maxSessionCount,
	NetworkSchedulerRef pNetworkScheduler,
	SessionFactory sessionFactory)
	: mServiceType(serviceType)
	, mMaxSessionCount(maxSessionCount)
	, mNetAddress(netAddress)
	, mpNetworkScheduler(std::move(pNetworkScheduler))
	, mSessionFactory(std::move(sessionFactory))
	, mSessionManager(maxSessionCount)
{
}

SessionRef Service::CreateSession()
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

eServiceType Service::GetServiceType() const
{
	return mServiceType;
}

NetAddress& Service::GetNetAddress()
{
	return mNetAddress;
}

NetworkSchedulerRef Service::GetNetworkScheduler() const
{
	return mpNetworkScheduler;
}

int32 Service::GetCurrentSessionCount()
{
	return mSessionManager.GetCurrentSessionCount();
}

int32 Service::GetMaxSessionCount() const
{
	return mMaxSessionCount;
}

ClientService::ClientService(const ClientServiceConfig& config)
	: Service(
		eServiceType::Client,
		config.netAddress,
		config.maxSessionCount,
		config.pNetworkScheduler,
		config.sessionFactory
	)
{}

bool ClientService::Start()
{
	const int32 sessionCount = GetMaxSessionCount();
	for (int32 i = 0; i < sessionCount; ++i)
	{
		const SessionRef pSession = CreateSession();
		if (pSession->Connect() == false)
		{
			return false;
		}
		(void)mSessionManager.AddSession(pSession);
	}

	return true;
}

void ClientService::CloseService()
{
}

bool ClientService::AddSession(const SessionRef& pSession)
{
	pSession->setSessionConnected();
	//const bool result = mSessionManager.AddSession(pSession);
	pSession->OnConnected();
	return true;
}

void ClientService::RemoveSession(const SessionRef& pSession)
{
	mSessionManager.RemoveSession(pSession);
}

ServerService::ServerService(const ServerServiceConfig& config)
	: Service(
		eServiceType::Server, 
		config.netAddress,
		config.maxSessionCount,
		config.pNetworkScheduler,
		config.sessionFactory)
	,mpListener()
	,mWaitQueueManager(config.maxWaitSessionCount)
	,mpSessionReaper()
{
	mpListener = cpp_net_engine::MakeShared<Listener>(config.acceptCount);
	mpSessionReaper = cpp_net_engine::MakeShared<SessionReaper>(config.sessionTimeoutMs);
}

bool ServerService::Start()
{
	if (mpListener == nullptr)
	{
		return false;
	}

	const ServerServiceRef pServerService = std::static_pointer_cast<ServerService>(shared_from_this());
	if (mpListener->StartAccept(pServerService) == false)
	{
		return false;
	}

	return true;
}

void ServerService::CloseService()
{
}

bool ServerService::AddSession(const SessionRef& pSession)
{
	if (mSessionManager.AddSession(pSession))
	{
		if (pSession->setSessionConnected() || pSession->setWaitingToConnected())
		{
			RegisterSessionReap(pSession);
			pSession->OnConnected();
			return true;
		}

		RemoveSession(pSession);
		pSession->Disconnect(eDisconnectReason::StateError);
		return false;
	}

	uint64 myTicket = 0;
	if (mWaitQueueManager.EnterWaitQueue(pSession, myTicket))
	{
		if (pSession->setSessionWaiting())
		{
			RegisterSessionReap(pSession);
			pSession->OnEnterWaitQueue(myTicket);
			return true;
		}

		pSession->Disconnect(eDisconnectReason::StateError);
		return false;
	}

	pSession->Disconnect(eDisconnectReason::ServerFull);
	return false;
}

void ServerService::RemoveSession(const SessionRef& pSession)
{
	mSessionManager.RemoveSession(pSession, true);
	admitWaitingSession();
}

bool ServerService::EnterWaitQueue(const SessionRef& pSession, uint64& outTicket)
{
	return mWaitQueueManager.EnterWaitQueue(pSession, outTicket);
}

SessionRef ServerService::DequeueWaitQueue()
{
	return mWaitQueueManager.DequeueWaitQueue();
}

void ServerService::RegisterSessionReap(const SessionRef& pSession)
{
	SessionWeak pSessionWeak = pSession;
	ServerServiceWeak pServerServiceWeak = std::static_pointer_cast<ServerService>(shared_from_this());
	mpNetworkScheduler->RegisterDelay([pSessionReaper = mpSessionReaper, pSessionWeak, pServerServiceWeak]()->void
		{
			pSessionReaper->ReapSession(pServerServiceWeak, pSessionWeak);
		}, mpSessionReaper->GetTimeoutMs());
}

void ServerService::RegisterAbortSession(const SessionRef& pSession)
{
	SessionWeak pSessionWeak = pSession;
	ServerServiceWeak pServerServiceWeak = std::static_pointer_cast<ServerService>(shared_from_this());
	mpNetworkScheduler->RegisterDelay([pSessionReaper = mpSessionReaper, pSessionWeak]()->void
		{
			pSessionReaper->AbortSession(pSessionWeak);
		}, SessionReaper::GetAbortTimeoutMs());
}

uint64 ServerService::GetWaitCount(const uint64 myTicket)
{
	const uint64 waitCount = mWaitQueueManager.GetWaitCount(myTicket);
	return waitCount;
}

void ServerService::admitWaitingSession()
{
	const SessionRef pWaitSession = DequeueWaitQueue();
	if (pWaitSession == nullptr)
	{
		mSessionManager.ReleaseKeepTicket();
		return;
	}

	if (!mSessionManager.AddWaitingSession(pWaitSession))
	{
		mSessionManager.ReleaseKeepTicket();
		pWaitSession->Disconnect(eDisconnectReason::StateError);
		return;
	}

	if (!pWaitSession->setWaitingToConnected())
	{
		mSessionManager.RemoveSession(pWaitSession);
		pWaitSession->Disconnect(eDisconnectReason::StateError);
		return;
	}

	pWaitSession->OnConnected();
}
