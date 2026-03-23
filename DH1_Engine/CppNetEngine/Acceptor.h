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

	IocpAcceptEvent mAcceptEvent;
	SessionRef mpSession; 
	ServerServiceRef mpServerService;
};

