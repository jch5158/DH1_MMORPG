#include "pch.h"
#include "SocketIocpObject.h"

#include "SocketUtils.h"

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
	const auto retHandle = reinterpret_cast<HANDLE>(mSocket);
	return retHandle;
}

bool SocketIocpObject::CreateSocket()
{
	if (mSocket == static_cast<SOCKET>(INVALID_SOCKET))
	{
		SOCKET createSocket = INVALID_SOCKET;
		if (SocketUtils::CreateTcpSocket(createSocket) == false)
		{
			return false;
		}

		mSocket = createSocket;
	}

	return true;
}

void SocketIocpObject::ForceCloseSocket()
{
	SocketUtils::Close(mSocket);
}
