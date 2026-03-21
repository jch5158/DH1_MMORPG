#pragma once
#include "IocpEvent.h"

class NetService;

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
	
	[[nodiscard]] bool Initialize(const SessionRef& pOwner, NetServiceRef pService);
	void Register();
	void Process() const;

private:

	IocpDisconnectEvent mDisconnectEvent;
	NetServiceRef mpService;
};

