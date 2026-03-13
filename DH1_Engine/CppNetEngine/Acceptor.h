#pragma once
#include "IocpEvent.h"

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

class Acceptor
{
public:

	Acceptor(const Acceptor&) = delete;
	Acceptor operator=(const Acceptor&) = delete;
	Acceptor(Acceptor&&) = delete;
	Acceptor operator=(Acceptor&&) = delete;

	explicit Acceptor(const int32 acceptorIndex);
	~Acceptor() = default;

	void SetOwner(ListenerRef pOwner);
	void SetService(ServiceRef pService);

	void Register();
	void Process();

private:

	IocpAcceptEvent mAcceptEvent;
	ServiceRef mpService;
};

