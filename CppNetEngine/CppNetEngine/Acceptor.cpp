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

Acceptor::~Acceptor()
{
	Clear();
}

void Acceptor::SetOwner(const ListenerRef& pOwner)
{
	mpOwner = pOwner;
	mAcceptEvent.SetOwner(pOwner);
}

void Acceptor::SetService(const ServiceRef& pService)
{
	mpService = pService;
}

void Acceptor::Register()
{
	const SessionRef pSession = mpService->CreateSession();
	if (pSession == nullptr)
	{
		NET_ENGINE_LOG_FATAL("Acceptor::Register() - mpServerService->CreateSession() is failed");
		Clear();
		return;
	}

	mAcceptEvent.SetSession(pSession);

	mAcceptEvent.ClearOverlapped();
	if (false == SocketUtils::AcceptEx(mpOwner->GetSocket(), pSession->GetSocket(), pSession->GetReceiveBufferPtr(), &mAcceptEvent))
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			mpOwner->mpErrorHandle(errorCode);
			pSession->Clear();
			Clear();
		}
	}
}

void Acceptor::Process()
{
	const SessionRef pSession = mAcceptEvent.GetClientSession();
	mAcceptEvent.ResetSession();

	if (SocketUtils::SetUpdateAcceptSocket(pSession->GetSocket(), mpOwner->GetSocket()) == false)
	{
		Clear();
		return;
	}

	SOCKADDR_IN sockAddr{};
	int32 sizeOfSockAddr = SIZE_OF_32(sockAddr);
	if (SOCKET_ERROR == getpeername(pSession->GetSocket(), reinterpret_cast<SOCKADDR*>(&sockAddr), &sizeOfSockAddr))
	{
		Clear();
		return;
	}

	pSession->setNetAddress(NetAddress(sockAddr));
	pSession->processConnect();
	Register();
}

void Acceptor::Clear()
{
	mpOwner.reset();
	mpService.reset();
	mAcceptEvent.ClearOverlapped();
	mAcceptEvent.ResetOwner();
}
