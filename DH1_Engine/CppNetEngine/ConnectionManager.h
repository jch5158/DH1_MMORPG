#pragma once

#include "LockFreeStack.h"
#include "SocketIocpObject.h"

class ConnectionManager final : public SocketIocpObject
{
public:

	using ErrorHandle = std::function<void(const uint32)>;

	explicit ConnectionManager(const int32 maxConnectionCount);
	virtual ~ConnectionManager() override = default;
	virtual void Dispatch(IocpEvent& iocpEvent, const uint32 numOfBytes) override;
	
	ConnectionManagerRef GetConnectionManagerRef();

	bool Connect(const ClientServiceRef& pService);
	void Close();
	void FreeConnection();

private:

	const int32 mMaxConnectionCount;
	std::atomic<int32> mCurrentConnectionCount;
	LockFreeStack<int32> mFreeIndexStack;
	Vector<ConnectorRef> mConnectors;
};