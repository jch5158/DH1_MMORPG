#pragma once

#include "IocpEvent.h"
#include "NetSendBuffer.h"

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
	void Send(const NetSendBufferRef& pSendBuffer);
	void Process(const uint32 numOfBytes);
	void Register();
	void Clear();
	void ClearEvent();

private:

	SessionRef mpOwner;
	IocpSendEvent mSendEvent;
	std::atomic<bool> mbSendRegistered;
	LockFreeQueue<NetSendBufferRef> mSendQueue;
};

