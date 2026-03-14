#include "pch.h"
#include "Service.h"
#include "Listener.h"
#include "Session.h"
#include "SessionReaper.h"
#include <utility>

Service::Service(
	const eServiceType serviceType,
	const NetAddress& netAddress,
	SessionFactory sessionFactory,
	IocpCoreRef pIocpCore,
	SessionReaperRef pSessionReaper,
	SessionManagerRef pSessionManager,
	WaitQueueManagerRef pWaitQueueManager)
	: mServiceType(serviceType)
	, mMaxSessionCount(pSessionManager->GetMaxSessionCount())
	, mNetAddress(netAddress)
	, mSessionFactory(std::move(sessionFactory))
	, mpIocpCore(std::move(pIocpCore))
	, mpSessionReaper(std::move(pSessionReaper))
	, mpSessionManager(std::move(pSessionManager))
	, mpWaitQueueManager(std::move(pWaitQueueManager))
{
}

void Service::CloseService()
{
	// TODO : CloseService
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

bool Service::AddSession(const SessionRef& pSession) const
{
	if (mpSessionManager->AddSession(pSession))
	{
		if (pSession->setSessionConnected() || pSession->setWaitingToConnected())
		{
			pSession->OnConnected();
			return true;
		}

		RemoveSession(pSession);
		pSession->Disconnect(eDisconnectReason::StateError);
	}
	else
	{
		uint64 myTicket;
		if (mpWaitQueueManager->EnterWaitQueue(pSession, myTicket))
		{
			if (pSession->setSessionWaiting())
			{
				pSession->OnEnterWaitQueue(myTicket);
				return true;
			}
			
			pSession->Disconnect(eDisconnectReason::StateError);
		}
	}

	return false;
}

void Service::RemoveSession(const SessionRef& pSession) const
{
	mpSessionManager->RemoveSession(pSession, true);
	admitWaitingSession();
}

bool Service::EnterWaitQueue(const SessionRef& pSession, uint64& outTicket) const
{
	return mpWaitQueueManager->EnterWaitQueue(pSession, outTicket);
}

SessionRef Service::DequeueWaitQueue() const
{
	return mpWaitQueueManager->DequeueWaitQueue();
}

void Service::RegisterSessionReap(const SessionRef& pSession) const
{
	SessionWeak pSessionWeak = pSession;
	mpIocpCore->RegisterDelay([pSessionReaper = mpSessionReaper, pSessionWeak]()->void
		{
			pSessionReaper->ReapSession(pSessionWeak);
		
		}, mpSessionReaper->GetTimeoutMs());
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

int32 Service::GetCurrentSessionCount() const
{
	return mpSessionManager->GetCurrentSessionCount();
}

int32 Service::GetMaxSessionCount() const
{
	return mMaxSessionCount;
}

bool Service::GetWaitCount(const uint64 myTicket, uint64& outWaitCount) const
{
	if (mpWaitQueueManager == nullptr)
	{
		return false;
	}

	outWaitCount = mpWaitQueueManager->GetWaitCount(myTicket);
	return true;
}

void Service::admitWaitingSession() const
{
	const SessionRef pWaitSession = DequeueWaitQueue();
	if (pWaitSession != nullptr)
	{
		if (mpSessionManager->AddWaitingSession(pWaitSession))
		{
			if (pWaitSession->setWaitingToConnected())
			{
				pWaitSession->OnConnected();
			}
			else
			{
				mpSessionManager->RemoveSession(pWaitSession);
				pWaitSession->Disconnect(eDisconnectReason::StateError);
			}
		}
		else
		{
			mpSessionManager->ReleaseKeepTicket();
			pWaitSession->Disconnect(eDisconnectReason::StateError);
		}
	}
	else
	{
		mpSessionManager->ReleaseKeepTicket();
	}
}

ClientService::ClientService(
	const NetAddress& targetAddress,
	SessionFactory pSessionFactory,
	NetworkSchedulerRef pNetworkScheduler,
	SessionManagerRef pSessionManager)
	: Service(
		eServiceType::Client, 
		targetAddress, 
		std::move(pSessionFactory), 
		std::move(pNetworkScheduler), 
		nullptr, 
		std::move(pSessionManager), 
		nullptr)
{
}

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

ServerService::ServerService(
	const NetAddress& targetAddress,
	SessionFactory pSessionFactory,
	ListenerRef pListener, 
	NetworkSchedulerRef pNetworkScheduler,
	SessionReaperRef pSessionReaper,
	SessionManagerRef pSessionManager, 
	WaitQueueManagerRef pWaitQueueManager)
	: Service(
		eServiceType::Server, 
		targetAddress, 
		std::move(pSessionFactory), 
		std::move(pNetworkScheduler), 
		std::move(pSessionReaper), 
		std::move(pSessionManager), 
		std::move(pWaitQueueManager))
	, mpListener(std::move(pListener))
{
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
