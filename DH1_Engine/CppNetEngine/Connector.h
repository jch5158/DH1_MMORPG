#pragma once
#include "IocpEvent.h"

class Service;

class IocpConnectEvent final : public IocpEvent
{
public:
	IocpConnectEvent();
};

class Connector
{
public:

	Connector(const Connector&) = delete;
	Connector operator=(const Connector&) = delete;
	Connector(Connector&&) = delete;
	Connector operator=(Connector&&) = delete;

	explicit Connector();
	~Connector() = default;

	[[nodiscard]] bool Initialize(const SessionRef& pOwner, ServiceRef pService);
	bool Register();
	void Process() const;

private:

	IocpConnectEvent mConnectEvent;
	ServiceRef mpService;
};

