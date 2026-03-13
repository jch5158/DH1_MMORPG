#pragma once
#include "IocpCore.h"
#include "SharedPtrUtils.h"
#include "SocketIocpObject.h"

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

	explicit Listener(const int32 acceptCount, ErrorHandle pErrorHandle);
	virtual ~Listener() override;

	virtual void Dispatch(IocpEvent& iocpEvent, uint32 numOfBytes) override;

	ListenerRef GetListenerRef();

	bool StartAccept(const ServerServiceRef& pServerService);
	void CloseAccept();

private:

	const int32 mAcceptCount;
	const ErrorHandle mpErrorHandle;
	Vector<AcceptorRef> mAcceptors;
};

