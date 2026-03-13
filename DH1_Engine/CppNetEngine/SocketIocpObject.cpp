#include "pch.h"
#include "SocketIocpObject.h"

SocketIocpObject::SocketIocpObject()
	:IocpObject()
	, mSocket(INVALID_SOCKET)
{
}

SOCKET SocketIocpObject::GetSocket() const
{
	return mSocket;
}

HANDLE SocketIocpObject::GetSocketHandle() const
{
	return reinterpret_cast<HANDLE>(mSocket);
}
