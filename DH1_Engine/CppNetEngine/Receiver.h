#pragma once

class NetReceiveBuffer;

class IocpReceiveEvent final : public IocpEvent
{
public:
	IocpReceiveEvent();
};

class Receiver
{
public:

	static constexpr int32 MAX_RECEIVE_WSABUF_SIZE = 2;

	Receiver(const Receiver&) = delete;
	Receiver operator=(const Receiver&) = delete;
	Receiver(Receiver&&) = delete;
	Receiver operator=(Receiver&&) = delete;

	explicit Receiver(const int32 receiveBufferSize);
	~Receiver() = default;

	[[nodiscard]] bool Initialize(const SessionRef& pOwner);
	void Reset();
	void Process(const uint32 numOfBytes);
	void Register();
	
private:

	IocpReceiveEvent mReceiveEvent;
	NetReceiveBuffer mNetReceiveBuffer;
};

