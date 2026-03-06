#include "pch.h"
#include "Connector.h"
#include "Service.h"
#include "Session.h"
#include "SocketUtils.h"

Connector::Connector()
	: mpOwner(nullptr)
	, mpService(nullptr)
	, mConnectEvent()
{
}

Connector::~Connector()
{
	Clear();
}

void Connector::SetOwner(const SessionRef& pOwner)
{
	mpOwner = pOwner;
	mConnectEvent.SetOwner(pOwner);
}

void Connector::SetService(const ServiceRef& pService)
{
	mpService = pService;
}

bool Connector::Register()
{
	if (mpOwner == nullptr || !mpOwner->IsDisconnected())
	{
		Clear();
		return false;
	}

	if (mpService == nullptr || mpService->GetServiceType() != eServiceType::Client)
	{
		Clear();
		return false;
	}

	if (SocketUtils::SetReuseAddress(mpOwner->GetSocket(), true) == false)
	{
		Clear();
		return false;
	}

	if (SocketUtils::BindAnyAddress(mpOwner->GetSocket(), 0) == false)
	{
		Clear();
		return false;
	}

	mConnectEvent.ClearOverlapped();
	if (false == SocketUtils::ConnectEx(mpOwner->GetSocket(), mpService->GetNetAddress().GetSockAddr(), &mConnectEvent))
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			mpOwner->OnError(errorCode);
			mpOwner->Disconnect(eDisconnectReason::SocketError);
			Clear();
			return false;
		}
	}

	return true;
}

void Connector::Process()
{
	Clear();
}

void Connector::Clear()
{
	mpOwner.reset();
	mpService.reset();
	mConnectEvent.ClearOverlapped();
	mConnectEvent.ResetOwner();
}
