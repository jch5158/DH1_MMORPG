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

	void SetOwner(const ListenerRef& pOwner);
	void SetService(const ServiceRef& pService);

	void Register();
	void Process();
	void Clear();

private:

	ListenerRef mpOwner;
	ServiceRef mpService;
	IocpAcceptEvent mAcceptEvent;
};

