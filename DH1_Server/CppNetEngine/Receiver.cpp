#include "pch.h"
#include "Receiver.h"

#include "Session.h"
#include "SocketUtils.h"

Receiver::Receiver()
	:mpOwner(nullptr)
	, mReceiveEvent()
	, mNetReceiveBuffer(90)
{
}

void Receiver::SetOwner(const SessionRef& pOwner)
{
	mpOwner = pOwner;
}

byte* Receiver::GetWritePtr() const
{
	return mNetReceiveBuffer.GetWritePtr();
}

void Receiver::Process(const uint32 numOfBytes)
{
	ClearEvent();

	if (mpOwner == nullptr || mpOwner->IsDisconnected())
	{
		return;
	}

	if (numOfBytes == 0)
	{
		mpOwner->Disconnect(eDisconnectReason::Closed);
		return;
	}

	mNetReceiveBuffer.MoveWritePos(static_cast<int32>(numOfBytes));

	mNetReceiveBuffer.LinearizeRead();

	const int32 useSize = mNetReceiveBuffer.GetUseSize();
	const int32 processLen = mpOwner->OnReceive(mNetReceiveBuffer.GetBufferPtr(), useSize);
	if (processLen < 0 || processLen > useSize)
	{
		mpOwner->Disconnect(eDisconnectReason::Kicked);
		return;
	}

	mNetReceiveBuffer.MoveReadPos(processLen);

	Register();
}

void Receiver::Register()
{
	if (mpOwner == nullptr || mpOwner->IsDisconnected())
	{
		return;
	}

	const int32 freeSize = mNetReceiveBuffer.GetFreeSize();
	if (freeSize == 0)
	{
		mpOwner->Disconnect(eDisconnectReason::Kicked);
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
	mReceiveEvent.SetOwner(mpOwner);
	if (SocketUtils::WsaReceive(mpOwner->GetSocket(), wsabufs, wsabufsLen, &mReceiveEvent) == false)
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			mpOwner->OnError(errorCode);
			mpOwner->Disconnect(eDisconnectReason::SocketError);
			ClearEvent();
			Clear();
		}
	}
}

void Receiver::Clear()
{
	mpOwner.reset();
	mNetReceiveBuffer.Clear();
}

void Receiver::ClearEvent()
{
	mReceiveEvent.ClearOverlapped();
	mReceiveEvent.ResetOwner();
}
