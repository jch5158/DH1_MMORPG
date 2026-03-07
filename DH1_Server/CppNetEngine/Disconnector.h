#pragma once

#include "IocpEvent.h"

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
	void Clear();

	void Register();
	void Process();

private:

	SessionRef mpOwner;
	IocpDisconnectEvent mDisconnectEvent;
};

