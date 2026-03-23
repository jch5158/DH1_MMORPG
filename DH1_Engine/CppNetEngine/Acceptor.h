#pragma once

class ServerService;

class IocpAcceptEvent final : public IocpEvent
{
public:
	explicit IocpAcceptEvent(const int32 acceptorIndex);
	~IocpAcceptEvent() = default;

	[[nodiscard]] int32 GetAcceptorIndex() const;

private:
	const int32 mAcceptorIndex;
};

class Acceptor
{
public:

	Acceptor(const Acceptor&) = delete;
	Acceptor operator=(const Acceptor&) = delete;
	Acceptor(Acceptor&&) = delete;
	Acceptor operator=(Acceptor&&) = delete;

	explicit Acceptor(const int32 acceptorIndex);
	~Acceptor() = default;

	[[nodiscard]] bool Initialize(const ListenerRef& pOwner, ServerServiceRef pService);

	void Register();
	void Process();

private:

	static constexpr int32 ACCEPT_ADDRESS_LENGTH = sizeof(SOCKADDR_IN) + 16;
	static constexpr int32 ACCEPT_BUFFER_SIZE = ACCEPT_ADDRESS_LENGTH * 2;

	IocpAcceptEvent mAcceptEvent;
	SessionRef mpSession;
	ServerServiceRef mpServerService;
	byte mAcceptAddressBuffer[ACCEPT_BUFFER_SIZE];
};

