#include "pch.h"
#include "Disconnector.h"
#include "Service.h"
#include "Session.h"
#include "SocketUtils.h"

Disconnector::Disconnector()
	: mDisconnectEvent()
	, mpService()
{
}

void Disconnector::SetOwner(const SessionRef& pOwner)
{
	if (pOwner == nullptr)
	{
		return;
	}

	mDisconnectEvent.SetOwner(pOwner);
}

void Disconnector::SetService(ServiceRef pService)
{
	if (pService == nullptr)
	{
		return;
	}

	mpService = std::move(pService);
}

void Disconnector::Register()
{
	const SessionRef pSession = std::static_pointer_cast<Session>(mDisconnectEvent.GetOwner());
	if (pSession == nullptr)
	{
		return;
	}

	mDisconnectEvent.ClearOverlapped();
	if (SocketUtils::DisconnectEx(pSession->GetSocket(), &mDisconnectEvent) == false)
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			pSession->OnError(errorCode);
		}
	}
}

void Disconnector::Process() const
{
	const SessionRef pSession = std::static_pointer_cast<Session>(mDisconnectEvent.GetOwner());
	if (pSession == nullptr)
	{
		return;
	}

	pSession->OnDisconnected();

	if (mpService != nullptr)
	{
		mpService->RemoveSession(pSession);
	}

	pSession->Clear();
}
