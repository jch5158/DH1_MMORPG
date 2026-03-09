#include "pch.h"
#include "Acceptor.h"

#include "Service.h"
#include "Session.h"
#include "SocketUtils.h"

Acceptor::Acceptor(const int32 acceptorIndex)
	: mAcceptEvent(acceptorIndex)
	, mpService()
{
}

void Acceptor::SetOwner(ListenerRef pOwner)
{
	if (pOwner == nullptr)
	{
		return;
	}

	mAcceptEvent.SetOwner(std::move(pOwner));
}

void Acceptor::SetService(ServiceRef pService)
{
	if (pService == nullptr)
	{
		return;
	}

	mpService = std::move(pService);
}

void Acceptor::Register()
{
	const ListenerRef pOwner = static_pointer_cast<Listener>(mAcceptEvent.GetOwner());
	if (pOwner == nullptr)
	{
		return;
	}

	if (mpService == nullptr)
	{
		return;
	}

	const SessionRef pSession = mpService->CreateSession();
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
