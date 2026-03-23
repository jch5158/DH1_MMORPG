#pragma once

class ConnectionPool final : public SocketIocpObject
{
public:

	using ErrorHandle = std::function<void(const uint32)>;

	explicit ConnectionPool(const int32 maxConnectionCount);
	virtual ~ConnectionPool() override = default;
	virtual void Dispatch(IocpEvent& iocpEvent, const uint32 numOfBytes) override;
	
	ConnectionPoolRef GetConnectionManagerRef();

	int32 GetMaxConnectionCount() const;
	bool Connect(const ClientServiceRef& pService);
	void Close();
	void FreeConnection();

private:

	const int32 mMaxConnectionCount;
	std::atomic<int32> mCurrentConnectionCount;
	LockFreeStack<int32> mFreeIndexStack;
	Vector<ConnectorRef> mConnectors;
};
