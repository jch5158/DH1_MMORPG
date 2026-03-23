#pragma once

class ConnectionPool final : public SocketIocpObject
{
public:

	explicit ConnectionPool(const int32 maxConnectionCount);
	virtual ~ConnectionPool() override = default;
	virtual void Dispatch(IocpEvent& iocpEvent, const uint32 numOfBytes) override;

	[[nodiscard]] int32 GetMaxConnectionCount() const;
	[[nodiscard]] int32 GetCurrentConnectionCount() const;
	bool Connect(const ClientServiceRef& pService);
	void Close();
	void FreeConnection();

private:

	const int32 mMaxConnectionCount;
	std::atomic<int32> mCurrentConnectionCount;
	LockFreeStack<int32> mFreeIndexStack;
	Vector<ConnectorRef> mConnectors;
};
