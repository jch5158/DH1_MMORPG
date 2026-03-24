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
	if (pSession == nullptr)
	{
		NET_ENGINE_LOG_ERROR("Connector::Register() - CreateSession returned nullptr");
		return false;
	}

	if (!pSession->IsDisconnectStarted())
	{
		NET_ENGINE_LOG_ERROR("Connector::Register() - Session is not in disconnect state");
		return false;
	}

	if (SocketUtils::SetReuseAddress(pSession->GetSocket(), true) == false)
	{
		NET_ENGINE_LOG_ERROR("Connector::Register() - SetReuseAddress failed, errorCode: {}", WSAGetLastError());
		return false;
	}

	if (SocketUtils::BindAnyAddress(pSession->GetSocket(), 0) == false)
	{
		const int32 bindError = WSAGetLastError();
		if (bindError != WSAEINVAL)
		{
			NET_ENGINE_LOG_ERROR("Connector::Register() - BindAnyAddress failed, errorCode: {}", bindError);
			return false;
		}
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
				NET_ENGINE_LOG_ERROR("Connector::Register() - ConnectEx failed, errorCode: {}", errorCode);
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

	int32 connectTime = -1;
	int32 optLen = SIZE_OF_32(connectTime);
	if (getsockopt(pSession->GetSocket(), SOL_SOCKET, SO_CONNECT_TIME, reinterpret_cast<char*>(&connectTime), &optLen) == SOCKET_ERROR || connectTime == -1)
	{
		pSession->ForceCloseSocket();
		if (mpService != nullptr)
		{
			mpService->OnSessionDisconnected(pSession);
		}
		return;
	}

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