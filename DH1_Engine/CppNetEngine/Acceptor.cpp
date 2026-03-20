#include "pch.h"
#include "Acceptor.h"
#include "Service.h"
#include "Session.h"
#include "SocketUtils.h"

IocpAcceptEvent::IocpAcceptEvent(const int32 acceptorIndex)
	: IocpEvent(eIocpEventType::Accept)
	, mAcceptorIndex(acceptorIndex)
{
}

int32 IocpAcceptEvent::GetAcceptorIndex() const
{
	return mAcceptorIndex;
}

Acceptor::Acceptor(const int32 acceptorIndex)
	: mAcceptEvent(acceptorIndex)
	, mpSession()
	, mpServerService()
{
}

bool Acceptor::Initialize(const ListenerRef& pOwner, ServerServiceRef pService)
{
	if (pOwner == nullptr || pService == nullptr)
	{
		return false;
	}

	mAcceptEvent.SetOwner(pOwner);
	mpServerService = std::move(pService);

	return true;
}

void Acceptor::Register()
{
	const ListenerRef pOwner = static_pointer_cast<Listener>(mAcceptEvent.GetOwner());
	if (pOwner == nullptr)
	{
		return;
	}

	if (mpServerService == nullptr)
	{
		return;
	}

	mpSession = mpServerService->CreateSession();
	if (mpSession == nullptr)
	{
		return;
	}

	mAcceptEvent.ClearOverlapped();
	if (false == SocketUtils::AcceptEx(pOwner->GetSocket(), mpSession->GetSocket(), mpSession->GetReceiveBufferPtr(), &mAcceptEvent))
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			NET_ENGINE_LOG_ERROR("Acceptor::Register() - SocketUtils::AcceptEx, errorCode : {}", errorCode);
		}
	}
}

void Acceptor::Process()
{
	do
	{
		const SessionRef pSession = mpSession;
		mpSession.reset();

		if (pSession == nullptr || mpServerService == nullptr)
		{
			break;
		}

		const ListenerRef pOwner = static_pointer_cast<Listener>(mAcceptEvent.GetOwner());
		if (pOwner == nullptr)
		{
			break;
		}

		if (SocketUtils::SetUpdateAcceptSocket(pSession->GetSocket(), pOwner->GetSocket()) == false)
		{
			break;
		}

		SOCKADDR_IN sockAddr{};
		int32 sizeOfSockAddr = SIZE_OF_32(sockAddr);
		if (SOCKET_ERROR == getpeername(pSession->GetSocket(), reinterpret_cast<SOCKADDR*>(&sockAddr), &sizeOfSockAddr))
		{
			break;
		}

		pSession->setNetAddress(NetAddress(sockAddr));
		
		if (pSession->setSessionConnected() == false)
		{
			pSession->Disconnect(eDisconnectReason::StateError);
			return;
		}

		if (mpServerService == nullptr)
		{
			pSession->Disconnect(eDisconnectReason::ServiceError);
			return;
		}

		mpServerService->OnSessionConnected(pSession);

	} while (false);

	Register();
}
