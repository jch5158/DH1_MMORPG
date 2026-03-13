#pragma once
#include "IocpEvent.h"

class IocpDisconnectEvent final : public IocpEvent
{
public:
	IocpDisconnectEvent();
};

class Disconnector
{
public:

	Disconnector(const Disconnector&) = delete;
	Disconnector operator=(const Disconnector&) = delete;
	Disconnector(Disconnector&&) = delete;
	Disconnector operator=(Disconnector&&) = delete;

	explicit Disconnector();
	~Disconnector() = default;
	
	void SetOwner(const SessionRef& pOwner);
	void SetService(ServiceRef pService);

	void Register();
	void Process() const;

private:

	IocpDisconnectEvent mDisconnectEvent;
	ServiceRef mpService;
};

