#include "pch.h"
#include "Disconnector.h"
#include "Service.h"
#include "Session.h"
#include "SocketUtils.h"

Disconnector::Disconnector()
	: mpOwner(nullptr)
	, mDisconnectEvent()
{
}

void Disconnector::SetOwner(const SessionRef& pOwner)
{
	mpOwner = pOwner;
	mDisconnectEvent.SetOwner(pOwner);
}

void Disconnector::Clear()
{
	mpOwner.reset();
}

void Disconnector::Register()
{
	if (mpOwner == nullptr)
	{
		Clear();
		return;
	}

	mDisconnectEvent.ClearOverlapped();
	mDisconnectEvent.SetOwner(mpOwner);
	if (SocketUtils::DisconnectEx(mpOwner->GetSocket(), &mDisconnectEvent) == false)
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			mpOwner->OnError(errorCode);
			Clear();
		}
	}
}

void Disconnector::Process()
{
	mDisconnectEvent.ClearOverlapped();
	mDisconnectEvent.ResetOwner();
	Clear();
}
