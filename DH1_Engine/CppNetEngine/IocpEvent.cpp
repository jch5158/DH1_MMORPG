#include "pch.h"
#include "IocpEvent.h"
#include "Session.h"

IocpEvent::IocpEvent(const eIocpEventType eventType)
	: OVERLAPPED{}
	, mEventType(eventType)
	, mCustomType(std::nullopt)
	, mpOwner()
{}

IocpEvent::IocpEvent(const int32 customType)
	:OVERLAPPED{}
	, mEventType(eIocpEventType::CustomEvent)
	, mCustomType(customType)
	, mpOwner()
{}

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

int32 IocpEvent::GetCustomType() const
{
	const int32 result = mCustomType.value_or(-1);
	return result;
}

void IocpEvent::SetOwner(const IocpObjectRef& pOwner)
{
	mpOwner = pOwner;
}

void IocpEvent::ResetOwner()
{
	mpOwner.reset();
}
