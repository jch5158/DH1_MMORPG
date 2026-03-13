#pragma once
#include "LockFreeQueue.h"
#include "SharedPtrUtils.h"

enum class eIocpEventType : uint8
{
	Accept,
	Connect,
	Disconnect,
	Send,
	Receive,
	ActorMessage
};

class IocpEvent : public OVERLAPPED
{
public:

	IocpEvent(const IocpEvent&) = delete;
	IocpEvent& operator=(const IocpEvent&) = delete;
	IocpEvent(IocpEvent&&) = delete;
	IocpEvent& operator=(IocpEvent&&) = delete;

	explicit IocpEvent(const eIocpEventType eventType);
	~IocpEvent() = default;

	void ClearOverlapped();
	[[nodiscard]] eIocpEventType GetEventType() const;
	[[nodiscard]] IocpObjectRef GetOwner() const;
	void SetOwner(const IocpObjectRef& pOwner);
	void ResetOwner();

private:

	const eIocpEventType mEventType;
	IocpObjectWeak mpOwner;
};