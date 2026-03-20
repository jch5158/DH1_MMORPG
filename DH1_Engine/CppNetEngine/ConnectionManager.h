#pragma once

#include "LockFreeStack.h"
#include "SocketIocpObject.h"

class ConnectionManager final : public SocketIocpObject
{
public:

	using ErrorHandle = std::function<void(const uint32)>;

	explicit ConnectionManager(const int32 connectCount);
	virtual ~ConnectionManager() override = default;
	virtual void Dispatch(IocpEvent& iocpEvent, const uint32 numOfBytes) override;
	
	ConnectionManagerRef GetConnectionManagerRef();

	bool Connect(const ClientServiceRef& pService);
	void Connect(const ClientServiceRef& pService, const int32 connectCount);
	void Close();

private:

	const int32 mConnectCount;
	LockFreeStack<int32> mFreeIndexStack;
	Vector<ConnectorRef> mConnectors;
};