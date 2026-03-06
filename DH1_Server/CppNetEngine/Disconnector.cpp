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

Disconnector::~Disconnector()
{
	Clear();
}

void Disconnector::SetOwner(const SessionRef& pOwner)
{
	mpOwner = pOwner;
	mDisconnectEvent.SetOwner(pOwner);
}

void Disconnector::Clear()
{
	mpOwner.reset();
	mDisconnectEvent.ClearOverlapped();
	mDisconnectEvent.ResetOwner();
}

void Disconnector::Register()
{
	if (mpOwner == nullptr)
	{
		Clear();
		return;
	}

	mDisconnectEvent.ClearOverlapped();
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
	Clear();
}
