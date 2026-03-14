#include "pch.h"
#include "Connector.h"
#include "Service.h"
#include "Session.h"
#include "SocketUtils.h"

IocpConnectEvent::IocpConnectEvent()
	:IocpEvent(eIocpEventType::Connect)
{
}

Connector::Connector()
	: mConnectEvent()
    , mpService()
{
}

bool Connector::Initialize(const SessionRef& pOwner, ServiceRef pService)
{
	if (pOwner == nullptr || pService == nullptr)
	{
		return false;
	}

	mConnectEvent.SetOwner(pOwner);
	mpService = std::move(pService);

	return true;
}

bool Connector::Register()
{
	const SessionRef pSession = std::static_pointer_cast<Session>(mConnectEvent.GetOwner());
	if (pSession == nullptr || !pSession->IsDisconnected())
	{
		return false;
	}

	if (mpService == nullptr || mpService->GetServiceType() != eServiceType::Client)
	{
		return false;
	}

	if (SocketUtils::SetReuseAddress(pSession->GetSocket(), true) == false)
	{
		return false;
	}

	if (SocketUtils::BindAnyAddress(pSession->GetSocket(), 0) == false)
	{
		return false;
	}

	mConnectEvent.ClearOverlapped();
	if (false == SocketUtils::ConnectEx(pSession->GetSocket(), mpService->GetNetAddress().GetSockAddr(), &mConnectEvent))
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			pSession->OnError(errorCode);
			pSession->Disconnect(eDisconnectReason::SocketError);
			return false;
		}
	}

	return true;
}

void Connector::Process() const
{
	const SessionRef pSession = std::static_pointer_cast<Session>(mConnectEvent.GetOwner());
	if (pSession == nullptr)
	{
		return;
	}

	if (mpService == nullptr)
	{
		return;
	}

	if (mpService->AddSession(pSession))
	{
		pSession->updateLastActivityMs();
		pSession->registerReap();
		pSession->registerReceive();
	}
	else
	{
		pSession->Disconnect(eDisconnectReason::ServerFull);
	}
}