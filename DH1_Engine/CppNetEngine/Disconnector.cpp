#include "pch.h"
#include "Disconnector.h"

IocpDisconnectEvent::IocpDisconnectEvent()
	:IocpEvent(eIocpEventType::Disconnect)
{
}

Disconnector::Disconnector()
	: mDisconnectEvent()
	, mpService()
{
}

bool Disconnector::Initialize(const SessionRef& pOwner, NetServiceRef pService)
{
	if (pOwner == nullptr || pService == nullptr)
	{
		return false;
	}

	mDisconnectEvent.SetOwner(pOwner);
	mpService = std::move(pService);
	return true;
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
			if (!SocketUtils::IsExpectedIocpError(errorCode))
			{
				pSession->OnError(errorCode);
			}

			Process();
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

	pSession->setSessionDisconnected();
	
	if (mpService != nullptr)
	{
		mpService->OnSessionDisconnected(pSession);
	}
}
