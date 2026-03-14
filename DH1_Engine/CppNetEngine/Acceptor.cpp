#include "pch.h"
#include "Acceptor.h"
#include "Service.h"
#include "Session.h"
#include "SocketUtils.h"

IocpAcceptEvent::IocpAcceptEvent(const int32 acceptorIndex)
	: IocpEvent(eIocpEventType::Accept)
	, mAcceptorIndex(acceptorIndex)
	, mpClientSession()
{
}

int32 IocpAcceptEvent::GetAcceptorIndex() const
{
	return mAcceptorIndex;
}

void IocpAcceptEvent::ResetSession()
{
	mpClientSession.reset();
}

void IocpAcceptEvent::SetSession(SessionRef pSession)
{
	if (pSession == nullptr)
	{
		return;
	}

	mpClientSession = std::move(pSession);
}

SessionRef IocpAcceptEvent::GetClientSession() const
{
	return mpClientSession;
}

Acceptor::Acceptor(const int32 acceptorIndex)
	: mAcceptEvent(acceptorIndex)
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

	const SessionRef pSession = mpServerService->CreateSession();
	if (pSession == nullptr)
	{
		return;
	}

	mAcceptEvent.ClearOverlapped();
	mAcceptEvent.SetSession(pSession);
	if (false == SocketUtils::AcceptEx(pOwner->GetSocket(), pSession->GetSocket(), pSession->GetReceiveBufferPtr(), &mAcceptEvent))
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			pOwner->mpErrorHandle(errorCode);
		}
	}
}

void Acceptor::Process()
{
	const ListenerRef pOwner = static_pointer_cast<Listener>(mAcceptEvent.GetOwner());
	const SessionRef pSession = mAcceptEvent.GetClientSession();
	mAcceptEvent.ResetSession();

	do
	{
		if (pOwner == nullptr)
		{
			break;
		}

		if (pSession == nullptr)
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
		pSession->processConnect();

	} while (false);

	Register();
}
