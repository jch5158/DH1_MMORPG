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
	~Connector() = default;

	void SetOwner(const SessionRef& pOwner);
	void SetService(ServiceRef pService);

	bool Register();
	void Process() const;

private:

	IocpConnectEvent mConnectEvent;
	ServiceRef mpService;
};

