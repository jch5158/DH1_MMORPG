#pragma once
#include "LockFreeQueue.h"
#include "SharedPtrUtils.h"

class Session;

enum class eIocpEventType : uint8
{
	Accept,
	Connect,
	Disconnect,
	Send,
	Receive
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

class IocpAcceptEvent final : public IocpEvent
{
public:
	explicit IocpAcceptEvent(const int32 acceptorIndex);

	[[nodiscard]] int32 GetAcceptorIndex() const;

	void ResetSession();
	void SetSession(SessionRef pSession);
	[[nodiscard]] SessionRef GetClientSession() const;


private:
	const int32 mAcceptorIndex;
	SessionRef mpClientSession;
};

class IocpConnectEvent final : public IocpEvent
{
public:
	IocpConnectEvent();
};

class IocpDisconnectEvent final : public IocpEvent
{
public:
	IocpDisconnectEvent();
};

class IocpReceiveEvent final : public IocpEvent
{
public:
	IocpReceiveEvent();
};

class IocpSendEvent final : public IocpEvent
{
public:
	IocpSendEvent();

	[[nodiscard]] Vector<NetSendBufferRef>& GetSendPendingBuffer();

private:
	Vector<NetSendBufferRef> mSendPendingBuffer; // 전송중인 버퍼
};