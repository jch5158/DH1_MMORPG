#pragma once
#include "IocpEvent.h"
#include "NetSendBuffer.h"

class IocpSendEvent final : public IocpEvent
{
public:
	IocpSendEvent();

	[[nodiscard]] Vector<NetSendBufferRef>& GetSendPendingBuffer();

private:
	Vector<NetSendBufferRef> mSendPendingBuffer; // 전송중인 버퍼
};

class Sender
{
public:

	static constexpr int32 MAX_SEND_WSABUF_SIZE = 64;

	Sender(const Sender&) = delete;
	Sender operator=(const Sender&) = delete;
	Sender(Sender&&) = delete;
	Sender operator=(Sender&&) = delete;

	explicit Sender();
	~Sender() = default;

	void SetOwner(const SessionRef& pOwner);
	void Send(NetSendBufferRef pSendBuffer);
	void Process(const uint32 numOfBytes);
	void Register();

private:

	IocpSendEvent mSendEvent;
	std::atomic<bool> mbSendRegistered;
	LockFreeQueue<NetSendBufferRef> mSendQueue;
};

