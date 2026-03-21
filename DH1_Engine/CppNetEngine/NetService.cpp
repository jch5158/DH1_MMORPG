#include "pch.h"
#include "NetService.h"
#include "ConnectionManager.h"
#include "Listener.h"
#include "NetworkScheduler.h"
#include "Session.h"
#include "SessionReaper.h"

NetService::NetService(
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
	return mMaxSessionCount;
}

ClientService::ClientService(const ClientServiceConfig& config)
	: NetService(
		eServiceType::Client,
		config.netAddress,
		config.maxSessionCount,
		config.pNetworkScheduler,
		config.sessionFactory
	)
{
	mpConnectionManager = cpp_net_engine::MakeShared<ConnectionManager>(config.maxSessionCount);
}

bool ClientService::Start()
{
	for (int32 i = 0; i < mMaxSessionCount; ++i)
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

ServerService::ServerService(const ServerServiceConfig& config)
	: NetService(
		eServiceType::Server, 
		config.netAddress,
		config.maxSessionCount,
		config.pNetworkScheduler,
		config.sessionFactory)
	,mpListener()
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
	mpNetworkScheduler->RegisterDelay([pSessionWeak]()->void
		{
			SessionReaper::AbortSession(pSessionWeak);
		}, SessionReaper::GetAbortTimeoutMs());
}