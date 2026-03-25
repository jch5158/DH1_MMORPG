#pragma once

class ServerService;
class IocpAcceptEvent;

class Listener final : public SocketIocpObject
{
public:

	friend class Acceptor;

	using ErrorHandle = std::function<void(const uint32)>;

	Listener(const Listener&) = delete;
	Listener& operator=(const Listener&) = delete;
	Listener(Listener&&) = delete;
	Listener& operator=(Listener&&) = delete;

	explicit Listener(const int32 acceptCount);
	virtual ~Listener() override;

	virtual void Dispatch(IocpEvent& iocpEvent, const uint32 numOfBytes) override;

	ListenerRef GetListenerRef();

	bool StartAccept(const ServerServiceRef& pServerService);
	void CloseAccept();

private:

	std::atomic<bool> mbClosed;
	const int32 mAcceptCount;
	Vector<AcceptorRef> mAcceptors;
};
