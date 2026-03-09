#pragma once
#include "IocpEvent.h"

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

