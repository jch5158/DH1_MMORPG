#pragma once
#include "IocpEvent.h"

class NetService;

class IocpConnectEvent final : public IocpEvent
{
public:
	explicit IocpConnectEvent(const int32 connectorIndex);

	[[nodiscard]] int32 GetConnectorIndex() const;

private:
	const int32 mConnectorIndex;
};

class Connector
{
public:

	Connector(const Connector&) = delete;
	Connector operator=(const Connector&) = delete;
	Connector(Connector&&) = delete;
	Connector operator=(Connector&&) = delete;

	explicit Connector(const int32 connectorIndex);
	~Connector() = default;

	[[nodiscard]] bool Initialize(const ConnectionManagerRef& pOwner, ClientServiceRef pService);
	bool Register();
	void Process();

private:

	IocpConnectEvent mConnectEvent;
	SessionRef mpSession;
	ClientServiceRef mpService;
};

