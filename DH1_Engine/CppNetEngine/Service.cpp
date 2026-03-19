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
	IocpCoreRef pIocpCore,
	SessionFactory sessionFactory)
	: mServiceType(serviceType)
	, mMaxSessionCount(maxSessionCount)
	, mNetAddress(netAddress)
	, mpIocpCore(std::move(pIocpCore))
	, mSessionFactory(std::move(sessionFactory))
	, mSessionManager(maxSessionCount)
{
}

SessionRef Service::CreateSession()
{
	SessionRef pSession = mSessionFactory();
	pSession->Initialize(shared_from_this());

	if (mpIocpCore->Register(pSession) == false)
	{
		pSession->Clear();
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

IocpCoreRef Service::GetIocpCore() const
{
	return mpIocpCore;
}

int32 Service::GetCurrentSessionCount()
{
	return mSessionManager.GetCurrentSessionCount();
}

int32 Service::GetMaxSessionCount() const
{
	return mMaxSessionCount;
}

ClientService::ClientService(
	const NetAddress& netAddress,
	const int32 maxSessionCount,
	NetworkSchedulerRef pNetworkScheduler,
	SessionFactory pSessionFactory)
	: Service(
		eServiceType::Client,
		netAddress,
		maxSessionCount,
		std::move(pNetworkScheduler),
		std::move(pSessionFactory))
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
	}

	return true;
}

void ClientService::CloseService()
{
}

bool ClientService::AddSession(const SessionRef& pSession)
{
	return true;
}

void ClientService::RemoveSession(const SessionRef& pSession)
{
}

ServerService::ServerService(
	const NetAddress& netAddress,
	const int32 acceptCount,
	const int32 maxSessionCount,
	const int32 maxWaitSize,
	const int64 sessionTimeoutMs,
	SessionFactory pSessionFactory,
	NetworkSchedulerRef pNetworkScheduler
)
	: Service(
		eServiceType::Server, 
		netAddress,
		maxSessionCount,
		std::move(pNetworkScheduler),
		std::move(pSessionFactory))
	,mpListener()
	,mWaitQueueManager(maxWaitSize)
	,mpSessionReaper()
{
	mpListener = cpp_net_engine::MakeShared<Listener>(acceptCount, [](const uint32)->void {});
	mpSessionReaper = cpp_net_engine::MakeShared<SessionReaper>(sessionTimeoutMs);
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
	}
	else
	{
		uint64 myTicket;
		if (mWaitQueueManager.EnterWaitQueue(pSession, myTicket))
		{
			if (pSession->setSessionWaiting())
			{
				RegisterSessionReap(pSession);
				pSession->OnEnterWaitQueue(myTicket);
				return true;
			}

			pSession->Disconnect(eDisconnectReason::StateError);
		}
	}

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
	mpIocpCore->RegisterDelay([pSessionReaper = mpSessionReaper, pSessionWeak, pServerServiceWeak]()->void
		{
			pSessionReaper->ReapSession(pServerServiceWeak, pSessionWeak);
		}, mpSessionReaper->GetTimeoutMs());
}

uint64 ServerService::GetWaitCount(const uint64 myTicket)
{
	const uint64 waitCount = mWaitQueueManager.GetWaitCount(myTicket);
	return waitCount;
}

void ServerService::admitWaitingSession()
{
	const SessionRef pWaitSession = DequeueWaitQueue();
	if (pWaitSession != nullptr)
	{
		if (mSessionManager.AddWaitingSession(pWaitSession))
		{
			if (pWaitSession->setWaitingToConnected())
			{
				pWaitSession->OnConnected();
			}
			else
			{
				mSessionManager.RemoveSession(pWaitSession);
				pWaitSession->Disconnect(eDisconnectReason::StateError);
			}
		}
		else
		{
			mSessionManager.ReleaseKeepTicket();
			pWaitSession->Disconnect(eDisconnectReason::StateError);
		}
	}
	else
	{
		mSessionManager.ReleaseKeepTicket();
	}
}
