#pragma once
#include "IocpEvent.h"
#include "NetReceiveBuffer.h"

class Receiver
{
public:

	static constexpr int32 MAX_RECEIVE_WSABUF_SIZE = 2;

	Receiver(const Receiver&) = delete;
	Receiver operator=(const Receiver&) = delete;
	Receiver(Receiver&&) = delete;
	Receiver operator=(Receiver&&) = delete;

	explicit Receiver();
	~Receiver();

	void SetOwner(const SessionRef& pOwner);
	[[nodiscard]] byte* GetWritePtr() const;

	void Process(const uint32 numOfBytes);
	void Register();
	void Clear();

private:

	SessionRef mpOwner;
	IocpReceiveEvent mReceiveEvent;
	NetReceiveBuffer mNetReceiveBuffer;
};

