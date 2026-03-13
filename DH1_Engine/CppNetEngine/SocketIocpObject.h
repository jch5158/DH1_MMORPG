#pragma once
#include "IocpCore.h"

class SocketIocpObject : public IocpObject
{
public:

	SocketIocpObject();
	virtual ~SocketIocpObject() override = default;

	SOCKET GetSocket() const;
	HANDLE GetSocketHandle() const;
	
	virtual void Dispatch(class IocpEvent& iocpEvent, const uint32 numOfBytes) override = 0;

protected:
	SOCKET mSocket;
};

