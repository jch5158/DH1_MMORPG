#include "pch.h"
#include "Connector.h"

IocpConnectEvent::IocpConnectEvent(const int32 connectorIndex)
	:IocpEvent(eIocpEventType::Connect)
	, mConnectorIndex(connectorIndex)
{}

int32 IocpConnectEvent::GetConnectorIndex() const
{
	return mConnectorIndex;
}

Connector::Connector(const int32 connectorIndex)
	: mConnectEvent(connectorIndex)
    , mpService()
{
}

bool Connector::Initialize(const ConnectionPoolRef& pOwner, ClientServiceRef pService)
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
	if (mpService == nullptr || mpService->GetServiceType() != eServiceType::Client)
	{
		return false;
	}

	const SessionRef pSession = mpService->CreateSession();
	if (pSession == nullptr || !pSession->IsDisconnectStarted())
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

	mpSession = pSession;
	mConnectEvent.ClearOverlapped();
	if (false == SocketUtils::ConnectEx(pSession->GetSocket(), mpService->GetNetAddress().GetSockAddr(), &mConnectEvent))
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			if (!SocketUtils::IsExpectedIocpError(errorCode))
			{
				pSession->OnError(errorCode);
			}

			pSession->Disconnect(eDisconnectReason::SocketError);
			return false;
		}
	}

	return true;
}

void Connector::Process()
{
	const SessionRef pSession = mpSession;
	if (pSession == nullptr)
	{
		return;
	}

	mpSession.reset();

	if (pSession->setSessionConnected() == false)
	{
		pSession->Disconnect(eDisconnectReason::StateError);
		return;
	}

	if (mpService == nullptr)
	{
		pSession->Disconnect(eDisconnectReason::ServiceError);
		return;
	}

	mpService->OnSessionConnected(pSession);
}