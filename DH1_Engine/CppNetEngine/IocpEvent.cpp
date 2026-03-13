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