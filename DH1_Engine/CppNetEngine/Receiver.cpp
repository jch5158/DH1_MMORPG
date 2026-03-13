#include "pch.h"
#include "Receiver.h"
#include "Session.h"
#include "SocketUtils.h"

IocpReceiveEvent::IocpReceiveEvent()
	:IocpEvent(eIocpEventType::Receive)
{
}

Receiver::Receiver()
	: mReceiveEvent()
	, mNetReceiveBuffer(NetReceiveBuffer::DEFAULT_BUFFER_SIZE)
{
}

void Receiver::SetOwner(const SessionRef& pOwner)
{
	if (pOwner == nullptr)
	{
		return;
	}

	mReceiveEvent.SetOwner(pOwner);
}

byte* Receiver::GetWritePtr() const
{
	return mNetReceiveBuffer.GetWritePtr();
}

void Receiver::Process(const uint32 numOfBytes)
{
	const SessionRef pOwner = static_pointer_cast<Session>(mReceiveEvent.GetOwner());
	if (pOwner == nullptr || pOwner->IsDisconnected())
	{
		return;
	}

	if (numOfBytes == 0)
	{
		pOwner->Disconnect(eDisconnectReason::Closed);
		return;
	}

	mNetReceiveBuffer.MoveWritePos(static_cast<int32>(numOfBytes));

	mNetReceiveBuffer.LinearizeRead();

	const int32 useSize = mNetReceiveBuffer.GetUseSize();
	const int32 processLen = pOwner->OnReceive(mNetReceiveBuffer.GetBufferPtr(), useSize);
	if (processLen < 0 || processLen > useSize)
	{
		pOwner->Disconnect(eDisconnectReason::Kicked);
		return;
	}

	mNetReceiveBuffer.MoveReadPos(processLen);

	Register();
}

void Receiver::Register()
{
	const SessionRef pOwner = static_pointer_cast<Session>(mReceiveEvent.GetOwner());
	if (pOwner == nullptr || pOwner->IsDisconnected())
	{
		return;
	}

	const int32 freeSize = mNetReceiveBuffer.GetFreeSize();
	if (freeSize == 0)
	{
		pOwner->Disconnect(eDisconnectReason::Kicked);
		return;
	}

	const int32 linearSize = mNetReceiveBuffer.GetLinearWriteSize();
	const int32 remainSize = freeSize - linearSize;

	WSABUF wsabufs[MAX_RECEIVE_WSABUF_SIZE]{};
	wsabufs[0].buf = reinterpret_cast<char*>(mNetReceiveBuffer.GetWritePtr());
	wsabufs[0].len = linearSize;

	int32 wsabufsLen = 1;
	if (remainSize != 0)
	{
		++wsabufsLen;
		wsabufs[1].buf = reinterpret_cast<char*>(mNetReceiveBuffer.GetBufferPtr());
		wsabufs[1].len = remainSize;
	}

	mReceiveEvent.ClearOverlapped();
	if (SocketUtils::WsaReceive(pOwner->GetSocket(), wsabufs, wsabufsLen, &mReceiveEvent) == false)
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			pOwner->OnError(errorCode);
			pOwner->Disconnect(eDisconnectReason::SocketError);
		}
	}
}
