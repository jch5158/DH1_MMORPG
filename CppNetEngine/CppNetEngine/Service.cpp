#include "pch.h"
#include "Service.h"
#include "Listener.h"
#include "Session.h"
#include "SessionReaper.h"
#include <utility>

Service::Service(const eServiceType serviceType, const NetAddress& netAddress, IocpCoreRef pIocpCore,
	ActorSchedulerRef pScheduler, SessionFactory pSessionFactory, SessionManagerRef pSessionManager, SessionReaperRef pSessionReaper, WaitQueueManagerRef pWaitQueueManager)
	: mServiceType(serviceType)
	, mMaxSessionCount(pSessionManager->GetMaxSessionCount())
	, mNetAddress(netAddress)
	, mpIocpCore(std::move(pIocpCore))
	, mpScheduler(std::move(pScheduler))
	, mpSessionFactory(std::move(pSessionFactory))
	, mpSessionManager(std::move(pSessionManager))
	, mpSessionReaper(std::move(pSessionReaper))
	, mpWaitQueueManager(std::move(pWaitQueueManager))
{
	if (mpSessionFactory == nullptr)
	{
		NET_ENGINE_LOG_FATAL("Service::Service - mSessionFactory is nullptr");
		CrashReporter::Crash();
	}
}

void Service::CloseService()
{
	// TODO : CloseService
}

SessionRef Service::CreateSession()
{
	SessionRef pSession = mpSessionFactory();
	pSession->setService(shared_from_this());
	pSession->setSessionEvent(shared_from_this());

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
	else if (mpWaitQueueManager != nullptr)
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
	if (mpWaitQueueManager == nullptr)
	{
		return false;
	}

	return mpWaitQueueManager->EnterWaitQueue(pSession, outTicket);
}

SessionRef Service::DequeueWaitQueue() const
{
	if (mpWaitQueueManager == nullptr)
	{
		return nullptr;
	}

	return mpWaitQueueManager->DequeueWaitQueue();
}

void Service::RegisterSessionReap(const SessionRef& pSession) const
{
	if (mpSessionReaper == nullptr)
	{
		return;
	}

	SessionWeak pSessionWeak = pSession;

	mpSessionReaper->PostDelay(mpScheduler, mpSessionReaper->GetTimeoutMs(), &SessionReaper::ReapSession, pSessionWeak);
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

ActorSchedulerRef Service::GetActorScheduler() const
{
	return mpScheduler;
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

ClientService::ClientService(const NetAddress& targetAddress, IocpCoreRef pIocpCore, ActorSchedulerRef pScheduler, SessionFactory pSessionFactory, SessionManagerRef pSessionManager)
	: Service(eServiceType::Client, targetAddress, std::move(pIocpCore), std::move(pScheduler), std::move(pSessionFactory), std::move(pSessionManager), nullptr, nullptr)
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

ServerService::ServerService(const NetAddress& targetAddress, ListenerRef pListener, IocpCoreRef pIocpCore, ActorSchedulerRef pScheduler, SessionFactory pSessionFactory, SessionManagerRef pSessionManager, SessionReaperRef pSessionReaper, WaitQueueManagerRef pWaitQueueManager)
	: Service(eServiceType::Server, targetAddress, std::move(pIocpCore), std::move(pScheduler), std::move(pSessionFactory), std::move(pSessionManager), std::move(pSessionReaper), std::move(pWaitQueueManager))
	, mpListener(std::move(pListener))
{
}

bool ServerService::Start()
{
	if (mpListener == nullptr)
	{
		return false;
	}

	const ServerServiceRef pServerService = static_pointer_cast<ServerService>(shared_from_this());
	if (mpListener->StartAccept(pServerService) == false)
	{
		return false;
	}

	return true;
}

void ServerService::CloseService()
{
}
