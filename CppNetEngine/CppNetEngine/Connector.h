#pragma once
#include "IocpEvent.h"

class Connector
{
public:

	Connector(const Connector&) = delete;
	Connector operator=(const Connector&) = delete;
	Connector(Connector&&) = delete;
	Connector operator=(Connector&&) = delete;

	explicit Connector();
	~Connector();

	void SetOwner(const SessionRef& pOwner);
	void SetService(const ServiceRef& pService);

	bool Register();
	void Process();
	void Clear();

private:

	SessionRef mpOwner;
	ServiceRef mpService;
	IocpConnectEvent mConnectEvent;
};

