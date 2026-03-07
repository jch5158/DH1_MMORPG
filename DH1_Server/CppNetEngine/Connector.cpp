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
		return false;
	}

	if (mpService == nullptr || mpService->GetServiceType() != eServiceType::Client)
	{
		return false;
	}

	if (SocketUtils::SetReuseAddress(mpOwner->GetSocket(), true) == false)
	{
		return false;
	}

	if (SocketUtils::BindAnyAddress(mpOwner->GetSocket(), 0) == false)
	{
		return false;
	}

	mConnectEvent.ClearOverlapped();
	mConnectEvent.SetOwner(mpOwner);
	if (false == SocketUtils::ConnectEx(mpOwner->GetSocket(), mpService->GetNetAddress().GetSockAddr(), &mConnectEvent))
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			mpOwner->OnError(errorCode);
			mpOwner->Disconnect(eDisconnectReason::SocketError);
			ClearEvent();
			Clear();
			return false;
		}
	}

	return true;
}

void Connector::Process()
{
	ClearEvent();
	Clear();
}

void Connector::Clear()
{
	mpOwner.reset();
	mpService.reset();
}

void Connector::ClearEvent()
{
	mConnectEvent.ClearOverlapped();
	mConnectEvent.ResetOwner();
}
