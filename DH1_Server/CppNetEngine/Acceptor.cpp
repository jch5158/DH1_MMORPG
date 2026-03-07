#include "pch.h"
#include "Acceptor.h"

#include "Service.h"
#include "Session.h"
#include "SocketUtils.h"

Acceptor::Acceptor(const int32 acceptorIndex)
	: mpOwner(nullptr)
	, mAcceptEvent(acceptorIndex)
{
}

void Acceptor::SetOwner(const ListenerRef& pOwner)
{
	mpOwner = pOwner;
}

void Acceptor::SetService(const ServiceRef& pService)
{
	mpService = pService;
}

void Acceptor::Register()
{
	if (mpOwner == nullptr)
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
	mAcceptEvent.SetOwner(mpOwner);
	mAcceptEvent.SetSession(pSession);
	if (false == SocketUtils::AcceptEx(mpOwner->GetSocket(), pSession->GetSocket(), pSession->GetReceiveBufferPtr(), &mAcceptEvent))
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			mpOwner->mpErrorHandle(errorCode);
			pSession->Clear();
		}
	}
}

void Acceptor::Process()
{
	const SessionRef pSession = mAcceptEvent.GetClientSession();

	mAcceptEvent.ClearOverlapped();
	mAcceptEvent.ResetOwner();
	mAcceptEvent.ResetSession();

	do
	{
		if (pSession == nullptr)
		{
			break;
		}

		if (mpOwner == nullptr)
		{
			pSession->Clear();
			break;
		}

		if (SocketUtils::SetUpdateAcceptSocket(pSession->GetSocket(), mpOwner->GetSocket()) == false)
		{
			pSession->Clear();
			break;
		}

		SOCKADDR_IN sockAddr{};
		int32 sizeOfSockAddr = SIZE_OF_32(sockAddr);
		if (SOCKET_ERROR == getpeername(pSession->GetSocket(), reinterpret_cast<SOCKADDR*>(&sockAddr), &sizeOfSockAddr))
		{
			pSession->Clear();
			break;
		}

		pSession->setNetAddress(NetAddress(sockAddr));
		pSession->processConnect();
	} while (false);

	Register();
}

void Acceptor::Clear()
{
	mpOwner.reset();
	mpService.reset();
}
