#include "pch.h"
#include "IocpEvent.h"

#include "Session.h"

IocpEvent::IocpEvent(const eIocpEventType eventType)
	:mEventType(eventType)
{
}

void IocpEvent::ClearOverlapped()
{
	OVERLAPPED::Internal = 0;
	OVERLAPPED::InternalHigh = 0;
	OVERLAPPED::Offset = 0;
	OVERLAPPED::OffsetHigh = 0;
	OVERLAPPED::hEvent = nullptr;
}

eIocpEventType IocpEvent::GetEventType() const
{
	return mEventType;
}

IocpObjectRef IocpEvent::GetOwner() const
{
	IocpObjectRef pIocpRef = mpOwner.lock();

	return pIocpRef;
}

void IocpEvent::SetOwner(const IocpObjectRef& pOwner)
{
	mpOwner = pOwner;
}

void IocpEvent::ResetOwner()
{
	mpOwner.reset();
}

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

void IocpAcceptEvent::SetSession(const SessionRef& pSession)
{
	if (pSession == nullptr)
	{
		return;
	}

	mpClientSession = pSession;
}

SessionRef IocpAcceptEvent::GetClientSession() const
{
	return mpClientSession;
}

IocpConnectEvent::IocpConnectEvent()
	:IocpEvent(eIocpEventType::Connect)
{
}

IocpDisconnectEvent::IocpDisconnectEvent()
	:IocpEvent(eIocpEventType::Disconnect)
{
}

IocpReceiveEvent::IocpReceiveEvent()
	:IocpEvent(eIocpEventType::Receive)
{
}

IocpSendEvent::IocpSendEvent()
	:IocpEvent(eIocpEventType::Send)
	, mSendPendingBuffer()
{
	mSendPendingBuffer.reserve(Sender::MAX_SEND_WSABUF_SIZE);
}

Vector<NetSendBufferRef>& IocpSendEvent::GetSendPendingBuffer()
{
	return mSendPendingBuffer;
}
