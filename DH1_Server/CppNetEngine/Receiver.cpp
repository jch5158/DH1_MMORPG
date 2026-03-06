#include "pch.h"
#include "Receiver.h"

#include "Session.h"
#include "SocketUtils.h"

Receiver::Receiver()
	:mpOwner(nullptr)
	, mReceiveEvent()
	, mNetReceiveBuffer()
{
}

Receiver::~Receiver()
{
	Clear();
}

void Receiver::SetOwner(const SessionRef& pOwner)
{
	mpOwner = pOwner;
	mReceiveEvent.SetOwner(pOwner);
}

byte* Receiver::GetWritePtr() const
{
	return mNetReceiveBuffer.GetWritePtr();
}

void Receiver::Process(const uint32 numOfBytes)
{
	if (mpOwner == nullptr || mpOwner->IsDisconnected())
	{
		Clear();
		return;
	}

	if (numOfBytes == 0)
	{
		mpOwner->Disconnect(eDisconnectReason::Kicked);
		Clear();
		return;
	}

	mNetReceiveBuffer.MoveWritePos(static_cast<int32>(numOfBytes));

	const int32 dataSize = mNetReceiveBuffer.GetUseSize();
	const int32 processLen = mpOwner->OnReceive(mNetReceiveBuffer.GetReadPtr(), static_cast<int32>(numOfBytes));
	if (processLen < 0 || processLen > dataSize)
	{
		mpOwner->Disconnect(eDisconnectReason::Kicked);
		Clear();
		return;
	}

	mNetReceiveBuffer.MoveReadPos(processLen);

	Register();
}

void Receiver::Register()
{
	if (mpOwner == nullptr || mpOwner->IsDisconnected())
	{
		Clear();
		return;
	}

	WSABUF wsabufs[MAX_RECEIVE_WSABUF_SIZE]{};

	const int32 freeSize = mNetReceiveBuffer.GetFreeSize();
	if (freeSize == 0)
	{
		mpOwner->Disconnect(eDisconnectReason::Kicked);
		Clear();
		return;
	}

	const int32 linearSize = mNetReceiveBuffer.GetLinearWriteSize();
	const int32 remainSize = freeSize - linearSize;

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
	if (SocketUtils::WsaReceive(mpOwner->GetSocket(), wsabufs, wsabufsLen, &mReceiveEvent) == false)
	{
		const int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			mpOwner->OnError(errorCode);
			mpOwner->Disconnect(eDisconnectReason::SocketError);
			Clear();
		}
	}
}

void Receiver::Clear()
{
	mpOwner.reset();
	mReceiveEvent.ClearOverlapped();
	mReceiveEvent.ResetOwner();
	mNetReceiveBuffer.Clear();
}
